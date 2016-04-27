#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PSKEL_MPPA
#define MPPA_SLAVE
#define BARRIER_SYNC_MASTER "/mppa/sync/128:1"
#define BARRIER_SYNC_SLAVE "/mppa/sync/[0..15]:2"
//#include "common.h"
//#include "../../include/interface_mppa.h"
#include "../../include/PSkel.h"

using namespace std;
using namespace PSkel;

struct Arguments
{
  int externCircle;
  int internCircle;
  float power;
};

namespace PSkel{
__parallel__ void stencilKernel(Array2D<int> input,Array2D<int> output,Mask2D<int> mask, Arguments arg, size_t h, size_t w){
    // if(h == 513 && w==127) {
    //   printf("Chamando...%d,%d\n", h,w);
    // }
    int numberA = 0;
    int numberI = 0;
    for (int z = 0; z < mask.size; z++) {
      if(z < arg.internCircle) {
        numberA += mask.get(z, input, h, w);
        //printf("A: %d\n", numberA);

      } else {
        numberI += mask.get(z, input, h, w);
        //printf("I: %d\n", numberI);
      }
    }
    float totalPowerI = numberI*(arg.power);// The power of Inhibitors
    //printf("Power of I: %f\n", totalPowerI);
    if(numberA - totalPowerI < 0) {
      output(h,w) = 0; //without color and inhibitor
      //printf("Zero\n");
    } else if(numberA - totalPowerI > 0) {
      output(h,w) = 1;//with color and active
      //printf("One\n");
    } else {
      output(h,w) = input(h,w);//doesn't change
      ////printf("K\n");
    }
  }
}


int CalcSize(int level){
  if (level == 1) {
    return 3;
  }
  if (level >= 1) {
    return CalcSize(level-1) + 2;
  }
  return 0;
}


int main(int argc,char **argv) {

   /**************Mask for test porpuses****************/
  int level = 1;
  int power = 2;
  int internCircle = pow(CalcSize(level), 2) - 1;
  int externCircle = pow(CalcSize(2*level), 2) - 1 - internCircle;
  int size = internCircle + externCircle;
  Mask2D<int> mask(size);
  int count = 0;
  for (int x = (level-2*level); x <= level; x++) {
    for (int y = (level-2*level); y <= level; y++) {
      if (x != 0 || y != 0) {
          mask.set(count, x, y);
          count++;
      }
    }
  }
 
  for (int x = (2*level-4*level); x <= 2*level; x++) {
    for (int y = (2*level-4*level); y <= 2*level; y++) {
      if (x != 0 || y != 0) {
        if (!(x <= level && x >= -1*level && y <= level && y >= -1*level)) {
          mask.set(count, x, y);
          count++;
        }
      }
    }
  }
  /*************************************************/

  /*********************Arg************************/
  Arguments arg;
  arg.power = power;
  arg.internCircle = internCircle;
  arg.externCircle = externCircle;
  /***********************************************/

  int nb_tiles = atoi(argv[0]);
  int width = atoi(argv[1]);
  int height = atoi(argv[2]);
  int cluster_id = atoi(argv[3]);
  int nb_threads = atoi(argv[4]);
  int iterations = atoi(argv[5]);
  int outteriterations = atoi(argv[6]);
  int itMod = atoi(argv[7]);
  

  Array2D<int> partInput(height, width);
  Array2D<int> output(height, width);
  Stencil2D<Array2D<int>, Mask2D<int>, Arguments> stencil(partInput, output, mask, arg);
  // if(iterations == 0)  {
  stencil.runMPPA(cluster_id, nb_threads, nb_tiles, outteriterations, itMod);
  // } else {
  //      stencil.runIterativeMPPA(cluster_id, nb_threads, nb_tiles, iterations);
  //}

}