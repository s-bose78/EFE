#include <stdlib.h>
#include <stdint.h>

extern char resp_output[];
extern int resp_output_len;
extern bool resp_decoded;   //return status of image decode

void sendImageFrAnalysis(const uint8_t *img_data, size_t img_len);