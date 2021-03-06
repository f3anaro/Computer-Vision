/**
 * Fast Threshold Median Filter
 *
 * @author: Lucas Kahlert <lucas.kahlert@tu-dresden.de>
 */
#include <iostream> // std::cout
#include <sstream>
#include <getopt.h> // getopt_long()
#include "opencv2/highgui/highgui.hpp" // cv:imread, cv::imshow, cv::waitKey

using namespace std;
using namespace cv;

// images
Mat image;
Mat image_median;
Mat image_filtered;

// filter parameters
int radius = 1;
int window_size;
int window_area;

// if the absolute difference between the image and the median 
// must be greater than this threshold to be filtered
int threshold = 42;

// histogram
int histogram[256];


/**
 * This function calculates the median of a single historgram
 * by summing all values from zero and stop if the sum reaches
 * the middle if the histogram width.
 *
 * The current index in the historgram is the median.
 */
static inline int histo_median(const int (&histogram)[256])
{
    int i = 0;

    for (int sum = 0; i < 256; i++) {
        sum += histogram[i];

        if (sum > window_area / 2) {
            break;
        }
    }

    return i;
}


void huang_median()
{
    // zero the histogram
    memset(histogram, 0, sizeof(histogram));

    // init histogram
    for (int row = 0; row < window_size; row++) {
        for (int col = 0; col < window_size; col++) {
            histogram[image.at<uchar>(row,col)]++;
        }
    }

    int row = radius;
    int median = 0;

    /*
     * We move in a snake like shape through the image. With that approach we
     * do not need to initialize the historgram for each row. 
     */
    while (true) {
        image_median.at<uchar>(row,radius) = histo_median(histogram);

        // move right
        for (int col = radius + 1; col < image.cols - radius; col++) {
            // remove left column
            for (int i = 0; i < window_size; i++) {
                histogram[image.at<uchar>(row - radius + i, col - radius - 1)]--;
            }

            // add right column
            for (int i = 0; i < window_size; i++) {
                histogram[image.at<uchar>(row - radius + i, col + radius)]++;
            }

            // calculate median for each channel
            image_median.at<uchar>(row,col) = histo_median(histogram);
        }


        // right edge. move down
        row++;

        // check if we have reached bottom row
        if (row >= image.rows - radius) {
            break;
        }

        // remove top row
        for (int col = image.cols - window_size; col < image.cols; col++) {
            histogram[image.at<uchar>(row - radius - 1, col)]--;
        }

        // add bottom row
        for (int col = image.cols - window_size; col < image.cols; col++) {
            histogram[image.at<uchar>(row + radius, col)]++;
        }

        image_median.at<uchar>(row,image.cols - radius - 1) = histo_median(histogram);

        // move left
        for (int col = image.cols - radius - 2; col >= radius; col--) {
            // remove right column
            for (int i = 0; i < window_size; i++) {
                histogram[image.at<uchar>(row - radius + i, col + radius + 1)]--;
            }

            // add left column
            for (int i = 0; i < window_size; i++) {
                histogram[image.at<uchar>(row - radius + i, col - radius)]++;
            }

            image_median.at<uchar>(row,col) = histo_median(histogram);
        }

        // left edge. move down
        row++;

        // check if we have reached bottom row
        if (row >= image.rows - radius) {
            break;
        }

        // remove top row
        for (int col = 0; col < window_size; col++) {
            histogram[image.at<uchar>(row - radius - 1, col)]--;
        }

        // add bottom row
        for (int col = 0; col < window_size; col++) {
            histogram[image.at<uchar>(row + radius, col)]++;
        }
    }
}


void on_trackbar(int, void*)
{
    // the radius could be changed via the trackbar
    // recalculate the median filter window
    window_size = 2 * radius + 1;
    window_area = window_size * window_size;

    image_median   = Mat::zeros(image.size(), image.type());
    image_filtered = Mat::zeros(image.rows - window_size, image.cols - window_size, image.type());

    huang_median();

    for (int row = radius; row < image_filtered.rows; row++) {
        for (int col = radius; col < image_filtered.cols; col++) {
            image_filtered.at<uchar>(row, col) = abs(image.at<uchar>(row + radius, col + radius) -
                                                     image_median.at<uchar>(row + radius, col + radius));
        }
    }

    for (int row = 0; row < image_filtered.rows; row++) {
        for (int col = 0; col < image_filtered.cols; col++) {
            if (image_filtered.at<uchar>(row,col) > threshold) {
                image_filtered.at<uchar>(row,col) = image_median.at<uchar>(row + radius, col + radius);
            } else {
                image_filtered.at<uchar>(row,col) = image.at<uchar>(row + radius, col + radius);
            }
        }
    }

    imshow("Threshold median filter", image_filtered);
}


int main(int argc, const char* argv[])
{
    if (argc != 2) {
        cout << "Usage: ./threshold_median_filter image" << endl;

        return 1;
    }

    image = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);

    if (image.empty()) {
        cerr << "Error: cannot read " << argv[1] << endl;

        return 1;
    }

    // create interactive scene
    namedWindow("Threshold median filter", 1);
    createTrackbar("radius",    "Threshold median filter", &radius,     50, on_trackbar);
    createTrackbar("threshold", "Threshold median filter", &threshold, 255, on_trackbar);

    // initial rendering
    on_trackbar(0, NULL);

    // wait indefinitly on a key stroke
    waitKey(0);

    return 0;
}