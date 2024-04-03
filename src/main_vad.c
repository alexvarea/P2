#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sndfile.h>
#include <math.h>
#include "vad.h"
#include "vad_docopt.h"

#define DEBUG_VAD 0x1

int main(int argc, char *argv[]) {


  /*  const int MVMAX = (int)tMVMAX*(1000)/frame_duration; //maximo de tramas en MV;
  const int MSMAX = (int)tMSMAX*(1000)/frame_duration;

  const int VMIN = (int)tVMIN*(1000)/frame_duration;
  const int SMIN = (int)tSMIN*(1000)/frame_duration;*/
  int verbose = 0; /* To show internal state of vad: verbose = DEBUG_VAD; */

  SNDFILE *sndfile_in, *sndfile_out = 0;
  SF_INFO sf_info;
  FILE *vadfile;
  int n_read = 0, i;
  int n_write = 0;

  VAD_DATA *vad_data;
  VAD_STATE state, last_state;

  
  float *buffer, *buffer_zeros;
  int frame_size;         /* in samples */
  float frame_duration;   /* in seconds */
  //float last_frame_size; //
  unsigned int t, last_t, last_defined_t; /* in frames */

  
  char	*input_wav, *output_vad, *output_wav;

  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "2.0");

  verbose    = args.verbose ? DEBUG_VAD : 0;
  input_wav  = args.input_wav;
  output_vad = args.output_vad;
  output_wav = args.output_wav;
  


  if (input_wav == 0 || output_vad == 0) {
    fprintf(stderr, "%s\n", args.usage_pattern);
    return -1;
  }

  /* Open input sound file */
  if ((sndfile_in = sf_open(input_wav, SFM_READ, &sf_info)) == 0) {
    fprintf(stderr, "Error opening input file %s (%s)\n", input_wav, strerror(errno));
    return -1;
  }

  if (sf_info.channels != 1) {
    fprintf(stderr, "Error: the input file has to be mono: %s\n", input_wav);
    return -2;
  }

  /* Open vad file */
  if ((vadfile = fopen(output_vad, "wt")) == 0) {
    fprintf(stderr, "Error opening output vad file %s (%s)\n", output_vad, strerror(errno));
    return -1;
  }

  /* Open output sound file, with same format, channels, etc. than input */
  if (output_wav) {
    if ((sndfile_out = sf_open(output_wav, SFM_WRITE, &sf_info)) == 0) {
      fprintf(stderr, "Error opening output wav file %s (%s)\n", output_wav, strerror(errno));
      return -1;
    }
  }

  /*float tMDMAXi = 200; //tiempo maximo indefinido en ms
  int MDMAX = (int)tMDMAXi*(1000)/frame_duration;*/
  vad_data = vad_open(sf_info.samplerate);


  
  /* Allocate memory for buffers */
  frame_size   = vad_frame_size(vad_data);
  buffer       = (float *) malloc(frame_size * sizeof(float));
  buffer_zeros = (float *) malloc(frame_size * sizeof(float));
  for (i=0; i< frame_size; ++i) buffer_zeros[i] = 0.0F;

  frame_duration = (float) frame_size/ (float) sf_info.samplerate;
  last_state = ST_UNDEF;

  
   //DECLARACION PARAMETROS
  /*float alfa1 = atof(args.alfa1);
  float alfa2 = atof(args.alfa2);
  float tMDMAXi = atof(args.MUMAX);*/
  //float DMINi; //minimo tiempo en estado definido

//PARAMETROS ENCONTRADOS CON SCRIPT DALE2
  float alfa1 = 0.1;
  float alfa2 = 10.3;
  float tMDMAXi = 230;
  tMDMAXi = tMDMAXi/1000;




 ; //tiempo maximo indefinido en s
  float aux1 = tMDMAXi/frame_duration;
  int MDMAX = (int)round(aux1);


  //printf("%.5f\n",frame_duration);
  
  set_param(vad_data, MDMAX);

  last_defined_t = 0;




  for (t = last_t = 0; ; t++) { /* For each frame ... */
    /* End loop when file has finished (or there is an error) */
    if  ((n_read = sf_read_float(sndfile_in, buffer, frame_size)) != frame_size) break;

    if (sndfile_out != 0) {
      sf_write_float(sndfile_out, buffer,frame_size);
      
    }
    state = vad(vad_data, buffer, alfa1, alfa2); //leemos estado
    if(t == 0){
      last_state = state;
      last_defined_t = t;
    }
    if(state != last_state){ //escribimos 0 en los tramos sin voz detectada
      if((state == ST_SILENCE)||(state == ST_VOICE)){
        fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_defined_t * frame_duration, t * frame_duration, state2str(last_state));
        if(last_state == ST_SILENCE){
          sf_seek(sndfile_out,last_defined_t * frame_duration*sf_info.samplerate,SEEK_SET);
          sf_write_float(sndfile_out,buffer_zeros,(t*frame_duration*sf_info.samplerate)- (last_defined_t * frame_duration*sf_info.samplerate));
        }

        last_defined_t = t;
        last_state = state;
      }
    }
    
    
    if (verbose & DEBUG_VAD) vad_show_state(vad_data, stdout);

    /* TODO: print only SILENCE and VOICE labels */
    /* As it is, it prints UNDEF segments but is should be merge to the proper value */
    
    last_t = t;

  }

  //Ultimo tramo de menos muestras que frame_size.
  if(state != last_state){
    if(state == ST_MV){
      fprintf(vadfile, "%.5f\t%.5f\t%s\n", t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(2));
      last_state = ST_VOICE;
    }else if(state == ST_MS){
      fprintf(vadfile, "%.5f\t%.5f\t%s\n", t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(1));
      last_state = ST_SILENCE;
      
    }
    
  }else{
    fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_defined_t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(state));
    last_state = state;
  }

  //Ponemos a cero el último tramo si es necesario
  if(last_state == ST_SILENCE){
    sf_seek(sndfile_out,last_defined_t * frame_duration*sf_info.samplerate,SEEK_SET);
    sf_write_float(sndfile_out,buffer_zeros,(t*frame_duration*sf_info.samplerate)- (last_defined_t * frame_duration*sf_info.samplerate));
  }

  
  state = vad_close(vad_data);
  /* TODO: what do you want to print, for last frames? */
  /*if (t != last_t)
    fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(state));*/

  /* clean up: free memory, close open files */
  free(buffer);
  free(buffer_zeros);
  sf_close(sndfile_in);
  fclose(vadfile);
  if (sndfile_out) sf_close(sndfile_out);
  return 0;
}
