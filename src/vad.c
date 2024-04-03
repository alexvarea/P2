#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MV", "MS"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  // feat.zcr = feat.p = feat.am = (float) rand()/RAND_MAX;
  feat.p = compute_power(x,N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;

 
  //vad_data->DMIN = DMIN; 

  
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x, float alfa1, float alfa2) { //nuevo parametro a la funcion

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  //printf("%d\n",vad_data->MDMAX);

  switch (vad_data->state) {
  case ST_INIT:
    vad_data->state = ST_SILENCE;
    vad_data->p0 = f.p;
    vad_data->k1 =  vad_data->p0 + alfa1;
    vad_data->k2 = vad_data->k1 + alfa2;

   
    vad_data->MDCOUNT = 0;
    //vad_data->DCOUNT = 0;
    
    break;

  case ST_SILENCE:
    
    vad_data->MDCOUNT = 0;
    if(f.p > vad_data->k1){ //ki > k1
      vad_data -> state = ST_MV;
      
    }
    break;

  case ST_MV:
    vad_data -> MDCOUNT++;
    
    if(f.p > vad_data->k2){ //ki > k2
      vad_data->state = ST_VOICE;
      vad_data->MDCOUNT = 0;

    }else if((vad_data -> MDCOUNT >= vad_data->MDMAX)||(f.p < vad_data -> k1)){ //timeout o ki < k1
      vad_data->state = ST_SILENCE; 
      vad_data->MDCOUNT = 0;
    }
    break;
  
  case ST_VOICE:
    //vad_data ->DCOUNT++;
    
    if(f.p < vad_data -> k2){ //ki < k2
      vad_data -> state = ST_MS;
    }
    break;
  
  case ST_MS:

    vad_data -> MDCOUNT++;

    if(f.p > vad_data-> k2){
      vad_data -> state = ST_VOICE;
    }else if((vad_data -> MDCOUNT >= vad_data->MDMAX)||(f.p < vad_data -> k1)){
      vad_data->state = ST_SILENCE;
    }
    break;
  }
  return vad_data -> state;

  
}
void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}

void set_param(VAD_DATA *vad_data,int MDMAXi){ //setter de parámetro
  vad_data->MDMAX = MDMAXi;

}
