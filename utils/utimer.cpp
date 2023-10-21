#include <iostream>
#include <chrono>


#define START(timename) auto timename = std::chrono::high_resolution_clock::now();
#define STOP(timename,elapsed)  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - timename).count();


class utimer {
  std::chrono::high_resolution_clock::time_point start;
  std::chrono::high_resolution_clock::time_point stop;
  std::string message; 
  using usecs = std::chrono::microseconds;
  using msecs = std::chrono::milliseconds;

private:
  long * us_elapsed;
  
public:

  utimer(const std::string m) : message(m),us_elapsed((long *)NULL) {
    start = std::chrono::high_resolution_clock::now();
  }
    
  utimer(const std::string m, long * us) : message(m),us_elapsed(us) {
    start = std::chrono::high_resolution_clock::now();
  }

  ~utimer() {
    stop =
      std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed =
      stop - start;
    auto musec =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    
    std::cout << message << " computed in " << musec << " usec " 
	      << std::endl;
    if(us_elapsed != NULL)
      (*us_elapsed) = musec;
  }
};
