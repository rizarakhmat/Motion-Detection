#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

class MotionDetector
{
private:
  // Reference background picture
  Mat background_picture;

  // Number of pixels of a frame of the video
  int pixels;

  // Treshold to declare motion in a frame
  int k;

  // Video capture OpenCV object to retrieve frames from the video
  VideoCapture vid_capture;

  // Init the background picture retrieving it and applying filters
  void init_background_picture()
  {
    // Retrieve the background picture as the first frame of the video
    vid_capture >> background_picture;

    // Error during capturing of the frame, throw exception
    if (background_picture.empty())
    {
      throw "The video file is empty";
    }

    // Apply greyscale and smooth to the background picture
    background_picture = smooth(greyscale(background_picture));
  }

  // Greyscale filter
  Mat greyscale(Mat frame)
  {
    // Instantiate greyscaled frame as a frame filled with zeros (8 bit unsigned int)
    Mat greyscaled = Mat::zeros(frame.rows, frame.cols, CV_8U);
    
    // For all the rows of the input frame
    for (int i = 0; i < frame.rows; i++)
    {
      // For all the columns of the input frame
      #pragma GCC unroll 8
      for (int j = 0; j < frame.cols; j++)
      {
        // Accumulator
        u_int16_t value = 0;

        // For all the channels of the input frame
        for (int k = 0; k < frame.channels(); k++)
        {
          // Sum up the three channels (R,G,B) of the single pixel into the accumulator
          value += frame.at<Vec3b>(i, j)[k];
        }

        // The corresponding pixel of the greyscaled frame is set as the average of the three channels
        greyscaled.at<u_int8_t>(i, j) = (value / 3);
      }
    }

    return greyscaled;
  }

  // Smooth filter
  Mat smooth(Mat frame)
  {
    // Instantiate smoothed frame as a frame filled with zeros (8 bit unsigned int)
    Mat smoothed = Mat::zeros(frame.rows, frame.cols, CV_8U);

    // Counter containing the valid neighbours of a pixel
    int counter;

    // For all the rows in the input frame
    for (int i = 0; i < frame.rows; i++)
    {
      // For all the columns of the input frame
      #pragma GCC unroll 8
      for (int j = 0; j < frame.cols; j++)
      {
        // Reset the counter
        counter = 0;

        // Accumulator
        u_int16_t value = 0;

        // For all the pixels at distance 1 from the target pixel, and the target
        for (int ii = i - 1; ii <= i + 1; ii++)
        {
          for (int jj = j - 1; jj <= j + 1; jj++)
          {
            // The neighbour is present
            if (ii >= 0 && ii < frame.rows && jj >= 0 && jj < frame.cols)
            {
              // Add the value of the neighbour to the accumulator
              value += frame.at<u_int8_t>(ii, jj);

              // Increase the counter
              counter++;
            }
          }
        }

        // Set the corresponding pixel of the smoothed frame as the average
        smoothed.at<u_int8_t>(i, j) = (value / counter);
      }
    }

    return smoothed;
  }

  // Compute the percentage of different pixels between the background picture and the input frame
  int difference(Mat frame)
  {
    // Subtract the input frame to the background picture
    Mat difference = background_picture - frame;

    // Counter of the number of different pixels
    int different_pixels = 0;

    // For all the rows and columns
    for (int i = 0; i < difference.rows; i++)
    {
      #pragma GCC unroll 8
      for (int j = 0; j < difference.cols; j++)
      {
        // If the value of the pixel is not zero, the frame and the background picture differ at the given pixel
        if (difference.at<u_int8_t>(i, j) != 0)
        {
          different_pixels++;
        }
      }
    }

    // Scale the number of different pixels to percentage
    return (different_pixels * 100 / pixels);
  }

public:
  // Motion detector constructor
  // filename: path to the input video
  // k: treshold to declare motion in a frame
  MotionDetector(string filename, int k) : k(k)
  {
    // Init the video capture utility
    vid_capture = VideoCapture(filename);
    
    // Error during opening of the video
    if (!vid_capture.isOpened())
    {
      throw "Unable to open video file";
    }

    // Init the background picture
    init_background_picture();

    // Set the number of pixels of a frame
    pixels = background_picture.rows * background_picture.cols;
  }

  // Destructor
  ~MotionDetector()
  {
    // Close the channel with the video
    vid_capture.release();
  }

  // Returns a frame from the video
  Mat get_frame()
  {
    Mat frame;

    // Capture the frame from the video
    vid_capture >> frame;

    return frame;
  }

  // Returns if the input frame contains motion
  bool motion_detected(Mat frame)
  {
    // Apply greyscale and smooth, compute the percentage of different pixels
    // with respect to the background picture and compare with the threshold
    return (difference(smooth(greyscale(frame))) >= k);
  }
};