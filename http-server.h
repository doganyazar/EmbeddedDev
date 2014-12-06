#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include "http-common.h"

/*Declare process*/
PROCESS_NAME(httpdProcess);

/*Type definition of the service callback*/
typedef bool (*ServiceCallback) (ConnectionState_t* pConnectionState);

/*
 *Setter of the service callback, this callback will be called in case of HTTP request.
 */
void http_server_set_service_callback( ServiceCallback callback );

/*
 * Returns query variable in the URL.
 * Returns true if the variable found, false otherwise.
 * Variable is put in the buffer provided.
 */
bool http_server_get_query_variable(
    ConnectionState_t* pConnectionState,
    const char *pcName,
    char* pcOutput,
    uint16_t nOutputSize);

/*
 * Returns variable in the Post Data.
 * Returns true if the variable found, false otherwise.
 * Variable is put in the buffer provided.
 */
bool http_server_get_post_variable(
    ConnectionState_t* pConnectionState,
    const char *pcName,
    char* pcOutput,
    uint16_t nOutputSize);

/*
 * Returns the value of the header name provided. Return NULL if header does not exist.
 */
const char* http_server_get_req_header_value(
    ConnectionState_t* pConnectionState, const char* pcHeaderName );

/*
 * Requests to save the header; only a number of headers will be saved
 * in buffer due to resource limitations.
 * Return true if the header will be saved, false otherwise.
 */
bool http_server_handle_req_header(const char* pcHeaderName);

/*
 * Adds the header name and value provided to the response.
 * Name of the header should be hardcoded since it is accessed from code segment
 * (not copied to buffer) whereas value of the header can be copied
 * depending on the relevant parameter. This is needed since some values may be
 * generated dynamically (ex: e-tag value)
 */
bool http_server_add_res_header(
    ConnectionState_t* pConnectionState,
    const char* pcName,
    const char* pcValue,
    bool bCopyValue);

/*
 * Getter method for the HTTP method (GET, POST, etc) of the request
 */
HttpMethod_t http_server_get_http_method(ConnectionState_t* pConnectionState);

/*
 * Setter for the status code (200, 201, etc) of the response.
 */
void http_server_set_http_status(ConnectionState_t* pConnectionState, StatusCode_t status);

/*
 * Puts the provided string in the connection buffer, used by a bunch of other functions
 * as well as rest module.
 */
char* http_server_put_in_conn_buffer(ConnectionState_t* pConnectionState, char* pcValue);

/*
 * Return a pointer to the response buffer in case the user wants to have direct access.
 */
char* http_server_get_res_buf(ConnectionState_t* pConnectionState);

/*
 * Copy the provided buffer contents to the response buffer.
 */
Error_t http_server_copy_to_response(
    ConnectionState_t* pConnectionState, const char* pcBuffer, uint16_t nSize);

/*
 * Copies the provided string to the end of the response buffer.
 */
Error_t http_server_concatenate_str_to_response(
    ConnectionState_t* pConnectionState, const char* pcBuffer);

/*
 * Returns pointer to the Post Data buffer.
 */
const char* http_server_get_post_data(ConnectionState_t* pConnectionState);

/*
 * Generates base url (ex: "http://172.16.79.0:8080") and copies it into the buffer provided.
 */
void http_server_get_base_url(char* pcOut);

/*
 * Returns the relative URL (ex: /temperature) of the resource accessed.
 */
const char* http_server_get_relative_url(ConnectionState_t* pConnectionState);

/*
 * Set the header "Content-Type" to the given media type.
 */
bool http_server_set_representation(
    ConnectionState_t* pConnectionState, MediaType_t mediaType);

#endif /*HTTPSERVER_H_*/
