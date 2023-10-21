#include "opencv2/opencv.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

#include <ff/ff.hpp>

#include "utils/utimer.cpp"
#include "utils/MotionDetector.cpp"

using namespace std;
using namespace cv;
using namespace ff;

// Global state containing the overall counter of frames with motion
atomic<int> number_of_frames_with_motion;

// Motion detector utility
unique_ptr<MotionDetector> motion_detector;

// Emitter of the farm
struct Emitter : ff_node_t<Mat>
{
  Mat *svc(Mat *)
  {
    // Capture frames from the input video and send to the workers
    while (true)
    {
      // Allocate the space for a frame
      Mat *frame = new Mat();

      // Retrieve frame
      *frame = motion_detector->get_frame();

      // If the frame is empty, the video is ended
      if (frame->empty())
        break;

      // Send frame to worker
      ff_send_out(frame);
    }
    return (EOS);
  }
};

// Worker of the farm
struct Worker : ff_node_t<Mat>
{
  Mat *svc(Mat *frame)
  {
    // Detect motion and, in case, increase local counter
    if (motion_detector->motion_detected(*frame))
      local_counter++;
    
    // Free space allocated to frame
    delete frame;

    return (GO_ON);
  }

  // Before terminating the worker
  void svc_end()
  {
    // Update atomically the global state summing up the local counter
    number_of_frames_with_motion += local_counter;
  }

  // Local counter of frames with motion
  int local_counter = 0;
};

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    cout << "Usage: ff_motion_detection video k pardegree" << endl;
    return -1;
  }

  // Path to the input video file
  string filename = argv[1];

  // Treshold to declare a frame with motion or not
  int k = atoi(argv[2]);

  // Parallelism degree
  int pardegree = atoi(argv[3]);

  // Pardegree must be at least 2 (emitter and 1 worker)
  if (pardegree < 2)
  {
    cout << "At least 2 concurrent activities are needed" << endl;
    return -1;
  }

  // Pardegree must be at most as the cores available on the machine
  if (pardegree > thread::hardware_concurrency())
  {
    cout << "At most " << thread::hardware_concurrency() << " concurrent activities are allowed" << endl;
    return -1;
  }

  // Init the global shared state
  number_of_frames_with_motion = 0;

  // Emitter of the farm
  Emitter emitter;

  // Vector of workers of the farm
  vector<unique_ptr<ff_node>> workers;

  // Instantiate (pardegree - 1) workers
  for (int i = 0; i < pardegree - 1; i++)
  {
    workers.push_back(make_unique<Worker>());
  }

  // Add workers and emitter to the farm
  ff_Farm<> farm(move(workers), emitter);

  // Remove collector of the farm
  farm.remove_collector();

  // On demand scheduling for the emitter
  farm.set_scheduling_ondemand();

  try
  {
    utimer u("FF motion detection");

    // Init motion detector utility
    motion_detector = make_unique<MotionDetector>(filename, k);

    // Execute and wait termination of the farm
    if (farm.run_and_wait_end() < 0)
    {
      error("running farm\n");
      return -1;
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