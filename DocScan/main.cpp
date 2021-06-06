//
//  main.cpp
//  DocScan
//
//  Created by Uygur Kıran on 14.05.2021.
//

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#ifdef __APPLE__
    #include "CameraAccess.h"
    CameraAccess access;
#endif

using namespace std;
using namespace cv;

// global
Mat imgOrj, imgThre, imgWarped, imgCropped;
vector<Point> initialPoints, docPoints;
float targetWidth = 420, targetHeight = 596;
vector<vector<Point>> conPolyCache;
int conPolyIdxCache;

// helper funcs
Mat preProcess(Mat img);
vector<Point> getContours(Mat img);
void drawPoints(vector<Point> points, Scalar color);
vector<Point> reorderPoints();
void warpImg(Mat img, vector<Point> points, float w, float h);

int main() {
    #ifdef __APPLE__
    // handle cam permission
    bool authRes = access.isGranted();
    if (authRes == false) {
        cout << "\nDocScan: ";
        cout << "\n*** Camera permission is needed. ***\n";
        return  0;
    }
    #endif
    
    // init capture
    VideoCapture cap(1);
    
    while (true) {
        cap.read(imgOrj);
        // scale down
        // resize(imgOrj, imgOrj, Size(), 0.7, 0.7);
        
        // preprocessing
        imgThre = preProcess(imgOrj);
        
        // get contours
        initialPoints = getContours(imgThre);
        if (initialPoints.size() > 0) docPoints = reorderPoints();
        
        // warp img
        if (docPoints.size() > 0) warpImg(imgOrj, docPoints, targetWidth, targetHeight);
        
        // crop
        int cropVal = 5;
        Rect cropRect(cropVal,cropVal,targetWidth-(2*cropVal), targetHeight-(2*cropVal));
        if (docPoints.size() > 0) imgCropped = imgWarped(cropRect);
        
        // draw on the orj
        drawPoints(docPoints, Scalar(0,0,255));
        
        // show
        imshow("DocScan", imgOrj);
        if (docPoints.size() > 0) imshow("DocScan Result", imgCropped);
        
        waitKey(1);
    }
    
    return 0;
}


// ///////////////////////
// Helper funcs
// ///////////////////////
Mat preProcess(Mat img) {
    Mat imgGray, imgBlur, imgEdges, imgDil;

    cvtColor(img, imgGray, COLOR_BGR2GRAY);
    GaussianBlur(imgGray, imgBlur, Size(3,3), 3, 0);
    Canny(imgBlur, imgEdges, 25, 75); // make contours visible
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3,3));
    dilate(imgEdges, imgDil, kernel);
    
    return imgDil;
}


vector<Point> getContours(Mat img) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    
    findContours(img, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    vector<vector<Point>> conPoly(contours.size());
    vector<Rect> boundRect(contours.size());
    vector<Point> corners; // of the biggest rect
    double maxArea = 0;
    
    // filter area
    for (int i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        
        if (area > 1000) {
            double perimeter = arcLength(contours[i], true);
            // find a rect by approx. curves
            approxPolyDP(contours[i], conPoly[i], 0.02 * perimeter, true);
            
            if (area > maxArea && conPoly[i].size() == 4) {
                // current biggest
                conPolyCache = conPoly;
                conPolyIdxCache = i;
                
                corners = {conPoly[i][0], conPoly[i][1], conPoly[i][2], conPoly[i][3]};
                maxArea = area;
            }
            
        }
    }
    
    return corners;
}

void drawPoints(vector<Point> points, Scalar color) {
    for (int i = 0; i < points.size(); i++) {
        circle(imgOrj, points[i], 10, color, FILLED);
        if (conPolyCache.size() > 0) {
            drawContours(imgOrj, conPolyCache, conPolyIdxCache, Scalar(255, 0, 255), 4);
        }
        // putText(imgOrj, to_string(i), points[i], FONT_HERSHEY_PLAIN, 4, color, 4);
    }
}

vector<Point> reorderPoints() {
    vector<Point> newPoints;
    int pCount = sizeof(initialPoints) / sizeof(initialPoints[0]);
    if (pCount < 3) { return {}; }
    // find min & max vals to find short & long edge
    vector<int> sumPoints, subtPoints;
    
    for (int i = 0; i < initialPoints.size(); i++) {
        sumPoints.push_back(initialPoints[i].x + initialPoints[i].y);
        subtPoints.push_back(initialPoints[i].x - initialPoints[i].y);
    }
    
    long minIdx = min_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin();
    long maxIdx = max_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin();

    long midIdx1 = max_element(subtPoints.begin(), subtPoints.end()) - subtPoints.begin();
    long midIdx2 = min_element(subtPoints.begin(), subtPoints.end()) - subtPoints.begin();
    
    newPoints.push_back(initialPoints[minIdx]); // p[0]
    newPoints.push_back(initialPoints[midIdx1]);
    newPoints.push_back(initialPoints[midIdx2]);
    newPoints.push_back(initialPoints[maxIdx]); // p[3]
    
    return newPoints;
}


void warpImg(Mat img, vector<Point> points, float w, float h) {
    Point2f src[4] = { points[0], points[1], points[2], points[3] };
    Point2f dst[4] = { {0.0f,0.0f}, {w,0.0f}, {0.0f,h}, {w,h} };
    
    Mat imgMatrix = getPerspectiveTransform(src, dst);
    warpPerspective(img, imgWarped, imgMatrix, Point(w,h));
}
