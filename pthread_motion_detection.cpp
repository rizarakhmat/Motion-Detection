#include "opencv2/opencv.hpp"
#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <atomic>

#include "utils/utimer.cpp"
#include "utils/MotionDetector.cpp"

using namespace std;
using namespace cv;

// Queues of frames. One per worker
vector<queue<Mat>> frames_queues;

// Motion detector utility
unique_ptr<MotionDetector> motion_detector;

// Global state containing the overall counter of frames with motion
atomic<int> number_of_frames_with_motion;

// Utility to pin a thread to a specific core
// target: target core
auto pin_thread = [](int target)
{
  cpu_set_t cpuset;
  pthread_t thread;

  thread = pthread_self();

  CPU_ZERO(&cpuset);
  CPU_SET(target, &cpuset);

  pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
};

// Function executed by the emitter
// nw: number of workers
void read_frames(int nw)
{
  // Emitter pinned to core 0
  pin_thread(0);

  // Index of the target worker
  int index = 0;

  // Capture frames from the input video and send to the workers
  while (true)
  {
    // Retrieve frame
    Mat frame = motion_detector->get_frame();

    // If the frame is empty, break immediately
    if (frame.empty())
    {
      // Send EOSes and stop
      for (int i = 0; i < nw; i++)
      {
        frames_queues.at(i).push(frame);
      }
      break;
    }

    // Seek for an idle worker
    while (!frames_queues.at(index).empty())
    {
      index = (index + 1) % nw;
    }

    // Send frame to the idle worker
    frames_queues.at(index).push(frame);

    index = (index + 1) % nw;
  }
  return;
}

// Function executed by the workers
// worker_number: index of the worker
void handle_frames(int worker_number)
{
  // Pin thread to specific core
  pin_thread(worker_number + 1);

  // Reference to its queue of frames
  queue<Mat> *frames_queue = &frames_queues.at(worker_number);

  // Local state represented by the counter of frames with motion the worker has detected
  int local_counter = 0;

  Mat frame;

  // Repeat until receives an EOS
  while (true)
  {
    // Spinlock until the queue contains a frame
    if (!frames_queue->empty())
    {
      // Retrieve the frame from the queue
      frame = frames_queue->front();
      frames_queue->pop();

      // EOS
      if (frame.empty())
      {
        break;
      }

      // Check for motion and eventually increase local counter
      if (motion_detector->motion_detected(frame))
        local_counter++;
    }
    else
    {
      asm("nop;");
    }
  }

  // Atomically update the global state
  number_of_frames_with_motion += local_counter;

  return;
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    cout << "Usage: pthread_motion_detection video k pardegree" << endl;
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

  try
  {
    utimer u("Pthread motion detection");

    // Vector containing threads references
    vector<thread> tids;

    // Instantiate the queues for the workers
    frames_queues.resize(pardegree - 1);

    // Init motion detector utility
    motion_detector = make_unique<MotionDetector>(filename, k);

    // Start emitter, informing there are (pardegree - 1) workers
    tids.push_back(thread(read_frames, pardegree - 1));

    // Start (pardegree - 1) workers
    for (int i = 0; i < pardegree - 1; i++)
    {
      tids.push_back(thread(handle_frames, i));
    }

    // Await threads termination
    for (thread &t : tids)
    { 
      t.join();
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