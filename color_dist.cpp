#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>

using namespace cv;
using namespace std;

const char* image_names = {"{1| |fruits.jpg|input image name}"};
const Scalar red = {0, 0, 255};

int max_dist = 1;
// Initially use a black pixel
Vec3b selected_pixel = {0, 0, 0};
Mat image;

inline double distance(Vec3b pixel)
{
    return sqrt(
        pow(selected_pixel[0] - pixel[0], 2) +
        pow(selected_pixel[1] - pixel[1], 2) +
        pow(selected_pixel[2] - pixel[2], 2)
    );
}

static void render()
{
    Mat binary_img = Mat::zeros(image.size(), CV_8UC1);

    for (int y = 0; y < image.rows; ++y) {
        for (int x = 0; x < image.cols; ++x) {
            int dist = distance(image.at<Vec3b>(y,x));
            // cout << dist << endl;

            if (dist < max_dist) {
                // cout << "w\n";
                binary_img.at<uchar>(y,x) = 255;
            }
        }
    }

    imshow("Binary map", binary_img);

    vector<Vec4i> hierarchy;
    vector<vector<Point> > contours;
    findContours(binary_img, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, Point(0,0));

    // Canvas for largest contour
    Mat drawing = Mat::zeros(binary_img.size(), CV_8UC3);

    if (!contours.empty()) {
        int i = 0;
        double max_area = 0;
        // index of the contour with the largest area
        int max_i = 0;

        while (i >= 0) {
            double area = contourArea(contours[i]);
            int child = hierarchy[i][2];

            while (child >= 0) {
                area -= contourArea(contours[child]);
                child = hierarchy[child][2];
            }

            if (max_area < area) {
                max_area = area;
                max_i = i;
            }

            // Go to next contour in the top hierarchy level
            i = hierarchy[i][0];
        }
        // Draw largest contour in red
        drawContours(drawing, contours, max_i, red, CV_FILLED, 8, hierarchy);
    }

    imshow("Contours", drawing);
}

static void select_threshold(int threshold, void*)
{
//    cout << "Select threshold " << threshold << endl;
    max_dist = threshold;

    render();
}

static void select_pixel(int event, int x, int y, int flags, void* userdata)
{
    // Only listen on left button clicks
    if  (event != EVENT_LBUTTONUP) {
        return;
    }

    // Do not track clicks outside the image
    if (x < 0 || y < 0 || x >= image.cols || y >= image.rows) {
        return;
    }

    // cout << x << " " << y << endl;
    selected_pixel = image.at<Vec3b>(y,x);

    render();
}

int main(int argc, char const *argv[])
{
    CommandLineParser parser(argc, argv, image_names);
    string filename = parser.get<string>("1");
    image = imread(filename, CV_LOAD_IMAGE_COLOR);

    if(image.empty()) {
        cout << "Cannot read image file:" << filename << endl;

        return -1;
    }

    // Create a new windows
    namedWindow("Color distance", 1);
    // namedWindow("Binary map", 1);

    // Create trackbar for distance range
    createTrackbar(
        "",
        "Color distance",
        &max_dist,
        255,
        select_threshold
    );


    //set the callback function for any mouse event
    setMouseCallback("Color distance", select_pixel, NULL);

    // Display image
    imshow("Color distance", image);

    // Wait for a key stroke indefinitly
    waitKey(0);

    return 0;
}
