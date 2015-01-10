#include <iostream>
#include <getopt.h> // getopt_long()

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace cv;

// function pointer to a function that can be used for block matching
typedef float(*match_t)(const int, const Mat&, const Mat&, const Point2i, const int, float*, int*, bool);


static void usage()
{
    cout << "Usage: ./stereo_match [options] left right" << endl;
    cout << "  options:" << endl;
    cout << "    -h, --help            Show this help message" << endl;
    cout << "    -r, --radius          Block radius for stereo matching. Default: 2" << endl;
    cout << "    -d, --max-disparity   Shrinks the range that will be used" << endl;
    cout << "                          for block matching. Default: 20" << endl;
    cout << "    -t, --target          Name of output file. Default: disparity.png" << endl;
    cout << "    -m, --median          Radius of the median filter applied to " << endl;
    cout << "                          the disparity map. If 0, this feature is " << endl;
    cout << "                          disabled. Default: 2" << endl;
    cout << "    -g, --ground-truth    Optimal disparity image. This activates the" << endl;
    cout << "                          search for the optimal block size for each pixel." << endl;
    cout << "                          The radius parameter will be used for the step" << endl;
    cout << "                          range [0, step]. For each element in the interval" << endl;
    cout << "                          there will be a match performed with radius = 2^step" << endl;
    cout << "                             2^step" << endl;
    cout << "    -c, --correlation     Method for computing correlation. There are:" << endl;
    cout << "                              ssd  sum of square differences" << endl;
    cout << "                              sad  sum of absolute differences" << endl;
    cout << "                              ccr  cross correlation" << endl;
    cout << "                          Default: sad" << endl;
}


static bool parsePositionalImage(Mat& image, const int channels, const string& name, int argc, char const *argv[])
{
    if (optind >= argc) {
        cerr << argv[0] << ": required argument: '" << name << "'" << endl;
        usage();

        return false;
    } else {
        image = imread(argv[optind++], channels);

        if (image.empty()) {
            cerr << "Error: Cannot read '" << argv[optind] << "'" << endl;

            return false;
        }
    }

    return true;
}


static float matchSSD(const int radius, const Mat& prev, const Mat& next, const Point2i center,
                      const int max_disparity, float* best_match, int* disparity, bool inverse)
{
    // we do not want to cast rapidly from int to float, because the SSD is an
    // integer
    int min_ssd = INT_MAX;

    // just looking into one direction
    // standard: patch from left image could be
    // found left from its position in right image
    int start = -max_disparity;
    int end   = 0;
    
    // inverse match (from right image to left one)    
    if (inverse) {
        start = 0;
        end = max_disparity;
    }
    
    if (center.x + start < 0) {
        start = -(center.x - radius);
    }
    if (center.x + end > next.cols) {
        end = next.cols - center.x;
    }

    // cout << "center.y = " << center.y << endl;
    // cout << "start    = " << start << endl;
    // cout << "center.y - start = " << (center.y - start) << endl;
    // cout << "end = " << end << endl;

    // Mat patch  = Mat::zeros(2 * radius + 1, 2 * radius + 1, CV_8UC1);
    // Mat search = Mat::zeros(2 * radius + 1, 2 * radius + 1, CV_8UC1);

    for (int col_offset = start; col_offset < end; col_offset += 1) {
        int ssd = 0;

        // walk through the patch
        for (int prow = -radius; prow <= radius; prow++) {
            for (int pcol = -radius; pcol <= radius; pcol++) {

                // patch.at<uchar>(prow + radius, pcol + radius)  = prev.at<uchar>(center.y + prow, center.x + pcol);
                // search.at<uchar>(prow + radius, pcol + radius) = next.at<uchar>(center.y + prow, center.x + pcol + col_offset);

                // grayscale images => uchar
                // patch - image
                int diff =    prev.at<uchar>(center.y + prow, center.x + pcol)
                           - next.at<uchar>(center.y + prow, center.x + pcol + col_offset);

                ssd  += diff * diff;
            }
        }


        // if (center.y > 200 && center.x > 120) {
        //     imshow("patch", patch);
        //     imshow("search", search);
        //     waitKey(0);

        //     cout << col_offset << ": " << ssd << endl;
        // }


        if (ssd < min_ssd) {
            min_ssd = ssd;
            *disparity = abs(col_offset);
        }

    }

    // if (center.y > 150) {

    //     Mat canvas;
    //     left.copyTo(canvas);

    //     for (int row = -radius; row < radius; row++) {
    //         for (int col = -radius; col < radius; col++) {
    //             // cout << row << ", " << col << endl;
    //             // canvas.at<uchar>(center.y + row, center.x + col) = left.at<uchar>(center.y + row, center.x + col);
    //             // canvas.at<uchar>(center.y + row, center.x + col) = 0;
    //             // canvas.at<uchar>(center.y + row, center.x + col) = right.at<uchar>(center.y + row, center.x + col);
    //             canvas.at<uchar>(center.y + row, center.x + col) = right.at<uchar>(center.y + row, center.x + col - (int) *disparity);
    //         }
    //     }
    //     imshow("canvas", canvas);
    //     waitKey(10);

    // }

    // cout << "disparity = " << *disparity << endl;

    // *best_match = (float) best_ssd;
}


static float matchSAD(const int radius, const Mat& prev, const Mat& next, const Point2i center,
                      const int max_disparity, float* best_match, int* disparity, bool inverse)
{
    // we do not want to cast rapidly from int to float, because the SSD is an
    // integer
    int min_sad = INT_MAX;

    // just looking into one direction
    int start = -max_disparity;
    int end   = 0;
    
    // inverse match (from right image to left one)    
    if (inverse) {
        start = 0;
        end = max_disparity;
    }
    
    if (center.x + start < 0) {
        start = -(center.x - radius);
    }
    if (center.x + end > next.cols) {
        end = next.cols - center.x;
    }

    for (int col_offset = start; col_offset < end; col_offset += 1) {
        int sad = 0;

        // walk through the patch
        for (int prow = -radius; prow <= radius; prow++) {
            for (int pcol = -radius; pcol <= radius; pcol++) {
                // grayscale images => uchar
                // patch - image
                int diff =   prev.at<uchar>(center.y + prow, center.x + pcol)
                           - next.at<uchar>(center.y + prow, center.x + pcol + col_offset);

                sad  += abs(diff);
            }
        }

        if (sad < min_sad) {
            min_sad = sad;
            *disparity = abs(col_offset);
        }

    }
}


static float matchCCR(const int radius, const Mat& prev, const Mat& next, const Point2i center,
                      const int max_disparity, float* best_match, int* disparity, bool inverse)
{
    const int patch_size = 2 * radius + 1;    
    int search_width     = max_disparity + 2 * radius;
    
    // top left corner of the search area
    Point2i search_corner;
    search_corner.y = center.y - radius;

    // should we search to the right side?
    if (!inverse) {
        search_corner.x = center.x - search_width + radius;

        if (search_corner.x < 0) {
            search_width += search_corner.x;
            search_corner.x = 0;
        }
    } else {
        search_corner.x = center.x -radius;

        if (search_corner.x + search_width >= next.cols) {
            search_width = (next.cols - 1) - search_corner.x;
        }
    }

    // cut off the patch and search area from the image
    Mat patch       = prev(Rect(center.x - radius, center.y - radius, patch_size,   patch_size));
    Mat search_area = next(Rect(search_corner.x,   search_corner.y,   search_width, patch_size));

    // perform normed cross correlation matching
    Mat result;
    matchTemplate(search_area, patch, result, CV_TM_CCORR_NORMED);

    // search for the best match
    float max_ccr = -INFINITY;

    for (int col = 0; col < result.cols; col++) {
        if (result.at<float>(0, col) > max_ccr) {
            max_ccr = result.at<float>(0, col);
            *disparity = col;
        }
    }

    // if we have searched on the left side, we have to 
    // 
    //    0   1   2   3   4   5 
    //  -------------------------
    //  |   |   |   | x |   |   |
    //  -------------------------
    //                    ^
    //                    |
    //                  center.x
    // 
    // we have to cout the cols from the right to the
    // maximum, in this example: 5 - 3 = 2
    // 
    if (!inverse) {
        *disparity = result.cols - *disparity;
    }
}


static void blockMatch(const Mat& left, const Mat& right, Mat& disparity,
                       const int radius, const int max_disparity,
                       match_t match_fn, bool inverse = false)
{
    disparity = Mat::zeros(left.size(), CV_8UC1);

    // walk through the left image
    for (int lrow = radius; lrow < left.rows - radius; lrow++) {

        // cout << "Row " << lrow << endl;
        cout << "." << flush;

        for (int lcol = radius; lcol < left.cols - radius; lcol++) {
            // cout << lrow << ", " << lcol << endl;

            int shift = 0;
            float result = 0;

            match_fn(radius, left, right, Point2i(lcol, lrow), max_disparity, &result, &shift, inverse);

            disparity.at<uchar>(lrow, lcol) = shift;                
        }
    }
}


/**
 * Left right consistency compensation
 */
static void lrcCompensation(Mat& disparity, const Mat& disparity_revert, const uint max_diff)
{
    Mat occlusion = Mat::zeros(disparity.size(), disparity.type());

    // detect occluded regions
    for (int row = 0; row < disparity.rows; row++) {
        for (int col = 0; col < disparity.cols; col++) {
            // diff.at<uchar>(row,col) = abs(disparity.at<uchar>(row,col) - disparity_revert.at<uchar>(row,col));
            uint diff = abs(disparity.at<uchar>(row,col) - disparity_revert.at<uchar>(row,col));

            if (diff > max_diff) {
                occlusion.at<uchar>(row,col) = 1;
                // disparity.at<uchar>(row,col) = 0; // optional
            }
        }
    }

    // search the nearest neighbor that is not occluded
    for (int row = 0; row < disparity.rows; row++) {
        for (int col = 0; col < disparity.cols; col++) {
            if (occlusion.at<uchar>(row, col)) {
                // search for the first pixel in the column that is not occluded
                for (int i = 0; i < disparity.cols / 2; i++) {
                    // search left
                    if (col - i >= 0 && disparity.at<uchar>(row, col - i) == 0) {
                        disparity.at<uchar>(row, col) = disparity.at<uchar>(row, col - i);
                        break;
                    }
                    // search right
                    if (col + i < disparity.cols && disparity.at<uchar>(row,col + i) == 0) {
                        disparity.at<uchar>(row, col) = disparity.at<uchar>(row, col + i);
                        break;
                    }
                }
            }
        }
    }
}


static void stereoMatch(const Mat& img_prev, const Mat& img_next, Mat& disparity,
                                const int radius, const int max_disparity, const int median_radius,
                                match_t match_fn) 
{
    Mat disparity_n2p; // from next to prev (for left right consistency -- LRC)

    blockMatch(img_prev, img_next, disparity,     radius, max_disparity, match_fn);
    blockMatch(img_next, img_prev, disparity_n2p, radius, max_disparity, match_fn, true);

    // Left right consistency
    lrcCompensation(disparity, disparity_n2p, max_disparity);

    // you can disable median filtering    
    if (median_radius) {
        // apply median filter
        medianBlur(disparity, disparity, 2 * median_radius + 1);
    } 
}


int main(int argc, char const *argv[])
{
    Mat img_prev;
    Mat img_next;
    Mat image;         // img_prev, but loaded with colors
    Mat disparity;     // from prev to next
    Mat ground_truth;  // optimal disparity map for the image pairs

    int radius = 2;
    int max_disparity = 20;
    int median_radius = 2;
    match_t match_fn = &matchSAD;
    string match_name = "sad";
    string target = "disparity.png";


    const struct option long_options[] = {
        { "help",           no_argument,       0, 'h' },
        { "radius",         required_argument, 0, 'r' },
        { "target",         required_argument, 0, 't' },
        { "max-disparity",  required_argument, 0, 'd' },
        { "median",         required_argument, 0, 'm' },
        { "ground-truth",   required_argument, 0, 'g' },
        { "correlation",    required_argument, 0, 'c' },
        0 // end of parameter list
    };

    // parse command line options
    while (true) {
        int index = -1;

        int result = getopt_long(argc, (char **) argv, "hr:t:d:m:g:c:", long_options, &index);

        // end of parameter list
        if (result == -1) {
            break;
        }

        switch (result) {
            case 'h':
                usage();
                return 0;

            case 'r':
                radius = atoi(optarg);
                if (radius < 0) {
                    cerr << argv[0] << ": Invalid radius " << optarg << endl;
                    return 1;
                }
                break;

            case 'd':
                max_disparity = atoi(optarg);
                if (max_disparity <= 0) {
                    cerr << argv[0] << ": Invalid maximal disparity " << optarg << endl;
                    return 1;
                }
                break;

            case 't':
                target = optarg;
                break;

            case 'm':
                median_radius = atoi(optarg);
                if (median_radius < 0) {
                    cerr << argv[0] << ": Invalid median radius " << optarg << endl;
                    return 1;
                }
                break;

            case 'g':
                ground_truth = imread(optarg, CV_LOAD_IMAGE_GRAYSCALE);
                break;

            case 'c':
                match_name = string(optarg);

                if (match_name == "ssd") {
                    match_fn = &matchSSD;
                } else if (match_name == "sad") {
                    match_fn = &matchSAD;
                } else if (match_name == "ccr") {
                    match_fn = &matchCCR;
                } else {
                    cerr << argv[0] << ": Invalid correlation method '" << optarg << "'" << endl;
                    return 1;
                }

                break;

            case '?': // missing option
                return 1;

            default: // unknown
                cerr << "unknown parameter: " << optarg << endl;
                break;
        }
    }

    // parse positional arguments
    if (!parsePositionalImage(image,    CV_LOAD_IMAGE_COLOR,     "frame1", argc, argv)) { return 1; }
    if (!parsePositionalImage(img_next, CV_LOAD_IMAGE_GRAYSCALE, "frame2", argc, argv)) { return 1; }

    // convert previous image into grayscale
    cvtColor(image, img_prev, CV_BGR2GRAY);

    cout << "Parameters: " << endl;
    cout << "    radius:        " << radius << endl;
    cout << "    match fn:      " << match_name << endl;
    cout << "    max-disparity: " << max_disparity << endl;
    cout << "    median radius: " << median_radius << endl;
    cout << "    target:        " << target << endl;

    // find optimal block sizes for each pixel if
    // the ground truth for disparity is given
    if (!ground_truth.empty()) {
        cout << "ground truth given" << endl;
        
        normalize(ground_truth, ground_truth, 0, 255, NORM_MINMAX);

        // imshow("GT", ground_truth);
        // waitKey(0);

        uint steps = radius;

        // try different block sizes 
        vector<Mat> disparities(steps);
        for (int i = 0; i < steps; i++) {
            // variable radius
            int var_radius = pow(2, i);

            cout << "block size: " << (2 * var_radius + 1) << endl;

            stereoMatch(img_prev, img_next, disparities[i], var_radius, max_disparity, median_radius, match_fn);

            // normalize result to [0, 255]
            normalize(disparities[i], disparities[i], 0, 255, NORM_MINMAX);
        }

        // compare different disparities to ground truth and save block size
        Mat opt_block_size = Mat(img_prev.size(), img_prev.type());
            disparity      = Mat(img_prev.size(), img_prev.type());

        for (int row = 0; row < img_prev.rows; row++) {
            for (int col = 0; col < img_prev.cols; col++) {
                int smallest_diff = INT_MAX;
                for (int i = 0; i < steps; i++) {
                    int diff = ground_truth.at<uchar>(row, col) - disparities[i].at<uchar>(row, col);
                    if (abs(diff) < smallest_diff) {
                        smallest_diff = diff;
                        opt_block_size.at<uchar>(row, col) = i;
                        disparity.at<uchar>(row, col) = disparities[i].at<uchar>(row, col);
                    }
                }
            }
        }

        // cout << opt_block_size << endl;
        // cout << disparities[0] << endl;

        // normalize(opt_block_size, opt_block_size, 0, 255, NORM_MINMAX);
        // imshow("Optimal block size for each pixel", opt_block_size);


        // disparity = disparities[0];

    } else {
        stereoMatch(img_prev, img_next, disparity, radius, max_disparity, median_radius, match_fn);
    }
     
    // normalize the result to [ 0, 255 ]
    normalize(disparity, disparity, 0, 255, NORM_MINMAX);

    try {
        imwrite(target, disparity);
    } catch (runtime_error& ex) {
        cerr << "Error: cannot save disparity map to '" << target << "'" << endl;

        return 1;
    }


    return 0;
}