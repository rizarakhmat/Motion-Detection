#include "opencv2/opencv.hpp"
#include <iostream>

#include "utils/utimer.cpp"
#include "utils/MotionDetector.cpp"

using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    cout << "Usage: sequential_motion_detection video k" << endl;
    return -1;
  }

  // Path to the input video file
  string filename = argv[1];
  
  // Treshold to declare a frame with motion or not
  int k = atoi(argv[2]);

  // Counter of the frames with motion detected
  int number_of_frames_with_motion = 0;

  Mat frame;

  try
  {
    utimer u("Sequential motion detection");

    // Init motion detector utility
    MotionDetector motion_detector(filename, k);

    // Capture frames from the input video
    while (true)
    {
      // Retrieve frame
      frame = motion_detector.get_frame();

      // If the frame is empty, break immediately
      if (frame.empty())
        break;

      // Check motion and, in case, increment counter
      if (motion_detector.motion_detected(frame))
        number_of_frames_with_motion++;
    }
  }
  catch (Exception e) // Error during acquisition of frames from the video
  {
    cerr << e.what() << endl;
    return -1;
  }

  // Print the final result
  cout << "Number of frames with motion detected: " << number_of_frames_with_motion << endl;

  return 0;
}