#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_system.h>
#include "cJSON.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "mbedtls/base64.h"
#include "esp_http_client.h"

const char *TAG="my_http_client";
//#define API_KEY "AIzaSyD4GoM4wIhgb1djZlifiM1DGmkNfCjXaAo"
#define API_KEY CONFIG_ESP_API_KEY

char resp_output[1024];
int resp_output_len;
bool resp_decoded;
static char *output_buffer;
static int output_len;

void parse_json(char* json_string) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "cJSON parse error");
        return;
    }
    
    resp_output_len = 0;
    resp_decoded = 0;
    cJSON *responses = cJSON_GetObjectItemCaseSensitive(root, "responses");
    if (cJSON_IsArray(responses)) {
        cJSON *response_item = NULL;
        cJSON_ArrayForEach(response_item, responses) {
            //ESP_LOGI(TAG, "--- Processing a new response ---"); // Indicate a new event/response
            cJSON *labelAnnotations = cJSON_GetObjectItemCaseSensitive(response_item, "labelAnnotations");
            if (cJSON_IsArray(labelAnnotations)) {
                cJSON *labelAnnotation = NULL;
                cJSON_ArrayForEach(labelAnnotation, labelAnnotations) {
                    cJSON *description = cJSON_GetObjectItemCaseSensitive(labelAnnotation, "description");
                    cJSON *score = cJSON_GetObjectItemCaseSensitive(labelAnnotation, "score");
                    if (cJSON_IsString(description) && cJSON_IsNumber(score)) {
                        //ESP_LOGI(TAG, "##Label - Description: %s, Score: %f", description->valuestring, score->valuedouble);
                        int bytes_written = snprintf(resp_output + resp_output_len,
                            sizeof(resp_output) - resp_output_len,
                            "Description: %s, Confidence Score: %f\n",
                            description->valuestring, score->valuedouble *100);

                            if (bytes_written > 0 && resp_output_len + bytes_written < sizeof(resp_output)) {
                            resp_output_len += bytes_written;
                            } else {
                            ESP_LOGE(TAG, "Buffer overflow or snprintf error");
                            cJSON_Delete(root);
                            
                            return;
                        }
                    }
                }
            }

            cJSON *localizedObjectAnnotations = cJSON_GetObjectItemCaseSensitive(response_item, "localizedObjectAnnotations");
            if (cJSON_IsArray(localizedObjectAnnotations)) {
                cJSON *localizedObjectAnnotation = NULL;
                cJSON_ArrayForEach(localizedObjectAnnotation, localizedObjectAnnotations) {
                    cJSON *name = cJSON_GetObjectItemCaseSensitive(localizedObjectAnnotation, "name");
                    cJSON *score = cJSON_GetObjectItemCaseSensitive(localizedObjectAnnotation, "score");
                    if (cJSON_IsString(name) && cJSON_IsNumber(score)) {
                        //ESP_LOGI(TAG, "--Object - Name: %s, Score: %f", name->valuestring, score->valuedouble);
                    }
                    cJSON *boundingPoly = cJSON_GetObjectItemCaseSensitive(localizedObjectAnnotation, "boundingPoly");
                    if(boundingPoly && cJSON_IsObject(boundingPoly)){
                        cJSON *normalizedVertices = cJSON_GetObjectItemCaseSensitive(boundingPoly, "normalizedVertices");
                        if(normalizedVertices && cJSON_IsArray(normalizedVertices)){
                            cJSON *normalizedVertex = NULL;
                            cJSON_ArrayForEach(normalizedVertex, normalizedVertices){
                                cJSON *x = cJSON_GetObjectItemCaseSensitive(normalizedVertex, "x");
                                cJSON *y = cJSON_GetObjectItemCaseSensitive(normalizedVertex, "y");
                                if(x && y && cJSON_IsNumber(x) && cJSON_IsNumber(y)){
                                    //ESP_LOGI(TAG, "Bounding Poly - x: %f, y: %f", x->valuedouble, y->valuedouble);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    resp_decoded = 1;
    //printf("%s", resp_output);
    cJSON_Delete(root);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI("HTTP", "HTTP_EVENT_ERROR err");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI("HTTP", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            memset(resp_output,0, sizeof(resp_output));
            ESP_LOGI("HTTP", "HTTP_EVENT_HEADER_SENT");
            break;

        case HTTP_EVENT_ON_DATA:
            if (evt->data_len) {
                //ESP_LOGI("HTTP", "HTTP_EVENT_BODY, len = %d", evt->data_len);
                //printf("%.*s\n", evt->data_len, (char*)evt->data);
                output_buffer = realloc(output_buffer, output_len + evt->data_len + 1);
                if (output_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                    return ESP_FAIL;
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
                output_buffer[output_len] = '\0'; // Null-terminate
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            //ESP_LOGI("HTTP", "HTTP_EVENT_ON_FINISH");
            //ESP_LOGI(TAG, "Response: %s", output_buffer);
            parse_json(output_buffer);
            free(output_buffer);
            output_buffer = NULL;
            output_len = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}

char *image_to_base64(const uint8_t *image_data, size_t image_size, size_t *out_base64_len) {
    size_t max_base64_len = ((image_size + 2) / 3) * 4 + 1; // Calculate maximum possible base64 length
    char *base64_encoded = (char *)heap_caps_malloc(max_base64_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); //Allocate from SPIRAM if available.
    if (base64_encoded == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for base64 encoded data");
        return NULL;
    }

    size_t base64_len;
    int ret = mbedtls_base64_encode((unsigned char *)base64_encoded, max_base64_len, &base64_len, image_data, image_size);

    if (ret != 0) {
        ESP_LOGE(TAG, "Base64 encoding failed with error %d", ret);
        free(base64_encoded);
        return NULL;
    }

    base64_encoded[base64_len] = '\0'; // Null-terminate the string
    *out_base64_len = base64_len;

    return base64_encoded;
}


// Function to send image to Google Vision API
void sendImageFrAnalysis(const uint8_t *img_data, size_t img_len) {
    char *base64_encoded_image;
    char *post_data = NULL;
    cJSON *root = NULL;

    //1. Init the client
    esp_http_client_config_t config = {
        .url = "https://vision.googleapis.com/v1/images:annotate?key="API_KEY,
        .event_handler = _http_event_handler,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    //2. Base64 encode the image
    size_t base64_len;
    base64_encoded_image = image_to_base64(img_data, img_len, &base64_len);
    if (base64_encoded_image == NULL) {
        ESP_LOGE(TAG, "Image to Base64 conversion failed.");
        goto err;
    }

    //3. Create the JSON request payload
    root = cJSON_CreateObject();
    cJSON *requests = cJSON_CreateArray();
    cJSON *request = cJSON_CreateObject();
    cJSON *image = cJSON_CreateObject();
    cJSON *content = cJSON_CreateString(base64_encoded_image); // Use the encoded image data
    cJSON *features = cJSON_CreateArray();
    
    // LABEL_DETECTION
    cJSON *labelFeature = cJSON_CreateObject();
    cJSON_AddStringToObject(labelFeature, "type", "LABEL_DETECTION");
    cJSON_AddNumberToObject(labelFeature, "maxResults", 5);
    cJSON_AddItemToArray(features, labelFeature);
    
    // OBJECT_LOCALIZATION
    cJSON *objectFeature = cJSON_CreateObject();
    cJSON_AddStringToObject(objectFeature, "type", "OBJECT_LOCALIZATION");
    cJSON_AddNumberToObject(objectFeature, "maxResults", 5);
    cJSON_AddItemToArray(features, objectFeature);
    
    cJSON_AddItemToObject(image, "content", content);
    cJSON_AddItemToObject(request, "image", image);
    cJSON_AddItemToObject(request, "features", features);
    cJSON_AddItemToArray(requests, request);
    cJSON_AddItemToObject(root, "requests", requests);

    post_data = cJSON_PrintUnformatted(root);
    //ESP_LOGI(TAG, "post_data size = %d \n",strlen(post_data));

    // 4. Send the HTTP Request
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_err_t http_err = esp_http_client_perform(client);
    if (http_err != ESP_OK) {
        ESP_LOGI(TAG, "HTTP Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(http_err));
    }

err:
    //5. Cleanup
    if (base64_encoded_image) 
        free(base64_encoded_image);
    // Deletes the entire JSON tree
    if (root)
        cJSON_Delete(root);
    if (post_data) 
        free(post_data);
    esp_http_client_cleanup(client);
}