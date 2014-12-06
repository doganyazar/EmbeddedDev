#include <stdio.h>
#include <ctype.h> //for isxdigit
#include <stdlib.h> //for atoi
#include <string.h>

#include "contiki.h"
#include "logger.h"

#include "http-server.h"

static ServiceCallback serviceCallback = NULL;

struct GlobalConnectionSettings_t
{
  //these header values will be stored only, global for all services,
  //may be converted to something service specific later
  const char* requestHeaderNames[MAX_REQUEST_HEADERS];
  uint8_t nNumberOfRequestHeaders;

} gGlobalConnectionSettings;

void http_server_set_service_callback( ServiceCallback callback )
{
  serviceCallback = callback;
}

bool http_server_add_res_header(ConnectionState_t* pConnectionState, const char* pcName, const char* pcValue, bool bCopyValue)
{
  bool bReturnValue = false;
  if ( pConnectionState->response.numOfResponseHeaders < MAX_RESPONSE_HEADERS )
  {
    pConnectionState->response.headers[pConnectionState->response.numOfResponseHeaders].name = (char*) pcName;
    if ( bCopyValue == false )
    {
      pConnectionState->response.headers[pConnectionState->response.numOfResponseHeaders].value = (char*) pcValue;
    }
    else
    {
      /*DY : FIX_ME : need to put boundary check*/
      pConnectionState->response.headers[pConnectionState->response.numOfResponseHeaders].value
        = http_server_put_in_conn_buffer(pConnectionState, (char*)pcValue);
    }

    LOG_DBG("Added header name:%s value:%s\n",
      pConnectionState->response.headers[pConnectionState->response.numOfResponseHeaders].name,
      pConnectionState->response.headers[pConnectionState->response.numOfResponseHeaders].value) ;

    pConnectionState->response.numOfResponseHeaders++;
    bReturnValue = true;
  }

  return bReturnValue;
}

//TEXT_PLAIN, TEXT_XML, APPLICATION_XML, APPLICATION_JSON, APPLICATION_WWW_FORM, APPLICATION_ATOM_XML
char* gMediaTypes[] =
{
  "text/plain",
  "text/xml",
  "application/xml",
  "application/json",
  "application/x-www-form-urlencoded",
  "application/atom+xml"
};

const char* getMediaTypeString(MediaType_t eMediaType)
{
  return gMediaTypes[eMediaType];
}

char* getDefaultStatusString(StatusCode_t statusCode)
{
  char* pReturnValue = NULL;
  switch(statusCode)
  {
    case 200:
      pReturnValue="OK";
      break;
    case 201:
      pReturnValue="Created";
      break;
    case 202:
      pReturnValue="Accepted";
      break;
    case 204:
      pReturnValue="No Content";
      break;
    case 304:
      pReturnValue="Not Modified";
      break;
    case 400:
      pReturnValue="Bad Request" ;
      break;
    case 404:
      pReturnValue="Not Found" ;
      break;
    case 405:
      pReturnValue="Method Not Allowed" ;
      break;
    case 406:
      pReturnValue="Not Acceptable" ;
      break;
    case 414:
      pReturnValue="Request-URI Too Long" ;
      break;
    case 415:
      pReturnValue="Unsupported Media Type" ;
      break;
    case 500:
      pReturnValue="Internal Server Error" ;
      break;
    case 501:
      pReturnValue="Not Implemented" ;
      break;
    case 503:
      pReturnValue="Service Unavailable" ;
      break;
    //DY : FIX_ME : will be removed later, put to catch the unhandled statuses.
    default:
      pReturnValue="$$$$$ BUG $$$$";
      break;
  }

  return pReturnValue;
}

char* http_server_get_res_buf(ConnectionState_t* pConnectionState)
{
  return pConnectionState->response.responseBuffer;
}

bool http_server_set_representation(ConnectionState_t* pConnectionState, MediaType_t mediaType)
{
  return http_server_add_res_header(pConnectionState, HTTP_HEADER_NAME_CONTENT_TYPE, getMediaTypeString(mediaType), false);
}

//need to be changed to handle binary data as well
//rest also uses this function to put url pattern values in it
//it is not good that rest alters connection buffer but for the sake of optimization
//this will stay like this for now.
char* http_server_put_in_conn_buffer( ConnectionState_t* pConnectionState, char* pcValue )
{
  char* pcBegin = pConnectionState->connBuffer + pConnectionState->connBuffUsedSize;
  //DY : FIX_ME : no size check yet, need to update
  strcpy( (pcBegin), pcValue );
  pConnectionState->connBuffUsedSize += strlen(pcValue) + 1;

  return pcBegin;
}

//Copied from mangoose http server
static size_t decode(
  const char *pSrc,
  size_t nSrcLen,
  char *pDst,
  size_t nDstLen,
  bool bIsForm)
{
  size_t  i, j;
  int a, b;
#define HEXTOI(x)  (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < nSrcLen && j < nDstLen - 1; i++, j++) {
    if (pSrc[i] == '%' &&
        isxdigit(* (unsigned char *) (pSrc + i + 1)) &&
        isxdigit(* (unsigned char *) (pSrc + i + 2))) {
      a = tolower(* (unsigned char *) (pSrc + i + 1));
      b = tolower(* (unsigned char *) (pSrc + i + 2));
      pDst[j] = ((HEXTOI(a) << 4) | HEXTOI(b)) & 0xff;
      i += 2;
    } else if (bIsForm && pSrc[i] == '+') {
      pDst[j] = ' ';
    } else {
      pDst[j] = pSrc[i];
    }
  }

  pDst[j] = '\0';  /* Null-terminate the destination */

  return ( i == nSrcLen );
}

//Copied from mangoose http server
static bool getVariable(
  const char *pName,
  const char *pBuffer,
  size_t nBufLen,
  char* pOutput,
  size_t nOutputLen,
  bool bDecodeType)
{
  bool bReturnValue = false;
  const char  *pStart = NULL, *pEnd = NULL, *pEndOfValue;
  size_t nVariableLen = 0;

  //initialize the output buffer first
  *pOutput = 0;

  nVariableLen = strlen(pName);
  pEnd = pBuffer + nBufLen;

  for (pStart = pBuffer; pStart + nVariableLen < pEnd; pStart++)
  {
    if ((pStart == pBuffer || pStart[-1] == '&') && pStart[nVariableLen] == '=' &&
        ! strncmp(pName, pStart, nVariableLen))
    {
      /* Point p to variable value */
      pStart += nVariableLen + 1;

      /* Point s to the end of the value */
      pEndOfValue = (const char *) memchr(pStart, '&', pEnd - pStart);
      if (pEndOfValue == NULL)
      {
        pEndOfValue = pEnd;
      }

      bReturnValue = decode(
                      pStart,
                      pEndOfValue - pStart,
                      pOutput,
                      nOutputLen,
                      bDecodeType);
    }
  }

  return bReturnValue;
}

bool http_server_get_query_variable(ConnectionState_t* pConnectionState, const char *pcName, char* pcOutput, uint16_t nOutputSize)
{
  bool bReturnValue = false;
  if ( pConnectionState->userData.queryString )
  {
    bReturnValue = getVariable(
                    pcName,
                    pConnectionState->userData.queryString,
                    pConnectionState->userData.queryStringSize,
                    pcOutput,
                    nOutputSize,
                    false);
  }

  return bReturnValue;
}

bool http_server_get_post_variable(ConnectionState_t* pConnectionState, const char *pcName, char* pcOutput, uint16_t nOutputSize)
{
  bool bReturnValue = false;
  if ( pConnectionState->userData.postData )
  {
    bReturnValue = getVariable(
                    pcName,
                    pConnectionState->userData.postData,
                    pConnectionState->userData.postDataSize,
                    pcOutput,
                    nOutputSize,
                    true);
  }

  return bReturnValue;
}

static bool isHttpMethodHandled(ConnectionState_t* pConnectionState, const char* method)
{
  bool bReturnValue = true;

  //LOG_DBG("isHttpMethodHandled--> method:%s\n",method);

  //other method types can be added here if needed
  if(strncmp(method, httpGetString, 3) == 0 )
  {
    pConnectionState->requestType = METHOD_GET;
  }
  else if(strncmp(method, httpHeadString, 4) == 0 )
  {
    pConnectionState->requestType = METHOD_HEAD;
  }
  else if ( strncmp(method, httpPostString, 4) == 0  )
  {
    pConnectionState->requestType = METHOD_POST;
  }
  else if ( strncmp(method, httpPutString, 3) == 0  )
  {
    pConnectionState->requestType = METHOD_PUT;
  }
  else if ( strncmp(method, httpDeleteString, 3) == 0  )
  {
    pConnectionState->requestType = METHOD_DELETE;
  }
  else
  {
    LOG_ERR("No Method supported : %s\nstate : %d\n",pConnectionState->inputBuf, pConnectionState->state);
    bReturnValue = false;
  }

  return bReturnValue;
}

static Error_t parseUrl(ConnectionState_t* pConnectionState, char* pUrl)
{
  Error_t error = NO_ERROR;
  bool bFullUrlPath = false;
  //even for default index.html there is / Ex: GET / HTTP/1.1
  if( pUrl[0] != '/' )
  {
    //if url is complete (http://...) rather than relative
    if (strncmp(pUrl, httpString, 4) != 0 )
    {
      LOG_ERR("Url not valid : %s \n",pUrl);
      error = URL_INVALID;
    }
    else
    {
      bFullUrlPath = true;
    }
  }

  if ( error == NO_ERROR )
  {
    char* urlBuffer = pUrl;
    if ( bFullUrlPath )
    {
      unsigned char numOfSlash = 0;
      do
      {
        urlBuffer = strchr( ++urlBuffer, '/' );

        LOG_DBG("Buffer : %s %d\n", urlBuffer,numOfSlash);

      } while ( urlBuffer && ++numOfSlash < 3 );

      //DY : pUrl : ayrica buffer null ise nooluyo burada bak.
    }

    LOG_DBG("Url to be put :%s\n", urlBuffer);
    //DY : FIX_ME : burada sigdigi kadarini almasi sorun, sondaki \0 da elenmis oluyo. Duzelt ilerde.
    //strncpy( pConnectionState->resourceUrl, urlBuffer, sizeof( pConnectionState->resourceUrl ) );

    if ( urlBuffer )
    {
      pConnectionState->resourceUrl = http_server_put_in_conn_buffer(pConnectionState, urlBuffer);

      if ( ( pConnectionState->userData.queryString
           = strchr( pConnectionState->resourceUrl, '?' ) ) )
      {
        *(pConnectionState->userData.queryString++) = 0;

        pConnectionState->userData.queryStringSize =
          strlen(pConnectionState->userData.queryString);
      }

      LOG_DBG("Query String Size:%d String:%s\n", pConnectionState->userData.queryStringSize , pConnectionState->userData.queryString );

      decode(
        pConnectionState->resourceUrl,
        (int) strlen(pConnectionState->resourceUrl),
        pConnectionState->resourceUrl,
        strlen(pConnectionState->resourceUrl) + 1,
        false);
    }
    else
    {
      error = URL_INVALID;
    }
  }

  return error;
}

bool http_server_handle_req_header(const char* pcHeaderName)
{
  bool bReturnValue = false;

  if ( gGlobalConnectionSettings.nNumberOfRequestHeaders < MAX_REQUEST_HEADERS )
  {
    gGlobalConnectionSettings.requestHeaderNames[gGlobalConnectionSettings.nNumberOfRequestHeaders] = pcHeaderName;
    gGlobalConnectionSettings.nNumberOfRequestHeaders++;
    bReturnValue = true;
  }

  return bReturnValue;
}

static const char* isRequestHeaderNeeded(const char* headerValue)
{
  LOG_DBG("isRequestHeaderNeeded %d %s-->\n", gGlobalConnectionSettings.nNumberOfRequestHeaders, headerValue);
  const char* pReturnValue = NULL;
  uint8_t nIndex = 0;

  for ( nIndex=0; !pReturnValue && nIndex < gGlobalConnectionSettings.nNumberOfRequestHeaders ; nIndex++ )
  {
    if ( !strcmp( headerValue ,gGlobalConnectionSettings.requestHeaderNames[nIndex]))
    {
      pReturnValue = gGlobalConnectionSettings.requestHeaderNames[nIndex];
    }
  }

  LOG_DBG("<-- isRequestHeaderNeeded\n");
  return pReturnValue;
}

const char* http_server_get_req_header_value( ConnectionState_t* pConnectionState, const char* pcHeaderName )
{
  LOG_DBG("http_server_get_req_header_value %d %s -->\n",pConnectionState->numOfRequestHeaders, pcHeaderName);
  char* pcReturnValue = NULL;
  uint8_t nIndex = 0;

  for ( nIndex=0; !pcReturnValue && nIndex < pConnectionState->numOfRequestHeaders ; nIndex++ )
  {
    if ( !strcmp( pcHeaderName, pConnectionState->headers[nIndex].name))
    {
      pcReturnValue = pConnectionState->headers[nIndex].value;
    }
  }

  LOG_DBG("<-- http_server_get_req_header_value\n");
  return pcReturnValue;
}

static void parseHeader(ConnectionState_t* pConnectionState, char* inputBuf)
{
  LOG_DBG("parseHeader--->\n");
  const char* headerName = NULL;

  char* delimiter = strchr(inputBuf, ':');
  if ( delimiter )
  {
    *delimiter++ = 0; //after increment delimiter will point space char

    headerName = isRequestHeaderNeeded(inputBuf);

    if ( headerName && delimiter )
    {
      char* buffer = delimiter;

      if( buffer[0] == ' ' )
      {
        buffer++;
      }

      pConnectionState->headers[pConnectionState->numOfRequestHeaders].name = (char*)headerName;
      pConnectionState->headers[pConnectionState->numOfRequestHeaders].value = http_server_put_in_conn_buffer(pConnectionState, buffer);

      pConnectionState->numOfRequestHeaders++;
    }
  }
}

static PT_THREAD( handleRequest( ConnectionState_t* pConnectionState ) )
{
  PSOCK_BEGIN(&(pConnectionState->sin));

  static Error_t error;
  const char* pContentLen = NULL;
  //const char* pContentType = NULL;

  error = NO_ERROR; //always reinit static variables due to protothreads

  LOG_INFO("Request--->\n");

  //read method
  PSOCK_READTO(&(pConnectionState->sin), ' ');

  if ( !isHttpMethodHandled(pConnectionState, pConnectionState->inputBuf) )
  {
    //method not handled
    http_server_set_http_status(pConnectionState, SERVER_ERROR_SERVICE_UNAVAILABLE);
    pConnectionState->state = STATE_OUTPUT;

    //PSOCK_CLOSE_EXIT(&(pConnectionState->sin));
  }
  else
  {
    //read until the end of url
    PSOCK_READTO(&(pConnectionState->sin), ' ' );

    //-1 is needed since it also includes space char
    if ( pConnectionState->inputBuf[ PSOCK_DATALEN(&(pConnectionState->sin)) - 1 ] != ' ' )
    {
      error = URL_TOO_LONG;
    }

    pConnectionState->inputBuf[ PSOCK_DATALEN(&(pConnectionState->sin)) - 1 ] = 0;

    LOG_DBG("Read URL:%s\n", pConnectionState->inputBuf);

    if ( error == NO_ERROR )
    {
      error = parseUrl(pConnectionState, pConnectionState->inputBuf);
    }

    if ( error != NO_ERROR )
    {
      if ( error == URL_TOO_LONG )
      {
        http_server_set_http_status(pConnectionState, CLIENT_ERROR_REQUEST_URI_TOO_LONG);
      }
      else
      {
        http_server_set_http_status(pConnectionState, CLIENT_ERROR_BAD_REQUEST);
      }

      pConnectionState->state = STATE_OUTPUT;
      //PSOCK_CLOSE_EXIT(&(pConnectionState->sin));
    }
    else
    {
      //read until the end of HTTP version - not used yet
      PSOCK_READTO(&(pConnectionState->sin), LINE_FEED_CHAR);

      LOG_DBG("After URL:%s\n", pConnectionState->inputBuf);

      //DY : FIX_ME : PSOCK_READTO takes just a single delimiter so I read till the end of line
      //but now it may not fit in the buffer. If PSOCK_READTO would take two delimiters,
      //we would have read until : and <CR> so it would not be blocked.

      //Read the headers and store the necessary ones
      do
      {
        //read the next line
        PSOCK_READTO(&(pConnectionState->sin), LINE_FEED_CHAR);
        pConnectionState->inputBuf[ PSOCK_DATALEN(&(pConnectionState->sin)) - 1 ] = 0;

        //if headers finished then stop the infinite loop
        if ( pConnectionState->inputBuf[0] == CARRIAGE_RETURN_CHAR || pConnectionState->inputBuf[0] == 0 )
        {
          LOG_DBG("Finished Headers!\n\n");
          break;
        }

        parseHeader(pConnectionState, pConnectionState->inputBuf);
      }
      while( true );

      pContentLen = http_server_get_req_header_value(pConnectionState, HTTP_HEADER_NAME_CONTENT_LENGTH);
      if ( pContentLen )
      {
        pConnectionState->userData.postDataSize = atoi(pContentLen);

        LOG_DBG("Post Data Size %d\n",pConnectionState->userData.postDataSize);
      }

      if ( pConnectionState->userData.postDataSize )
      {
        static uint16_t readBytes = 0;
        //init the static variable again
        readBytes = 0;

        //DY : FIX_ME : 1 opt here can be to use the rest of conn buffer if there is space.
        pConnectionState->userData.postData = (char*)malloc(pConnectionState->userData.postDataSize + 1);

        if ( pConnectionState->userData.postData )
        {
          LOG_DBG("Post Data MAlloc OK:###%x###\n",(unsigned int)pConnectionState->userData.postData);
          //pConnectionState->userData.postData = pointConnBuffer(pConnectionState);

          do
          {
            PSOCK_READBUF(&(pConnectionState->sin));

            //null terminate the buffer in case it is a string.
            pConnectionState->inputBuf[PSOCK_DATALEN(&(pConnectionState->sin))] = 0;

            LOG_DBG("After While ###%x### \n", (unsigned int)pConnectionState->userData.postData);

            LOG_DBG("read: #%d (of %d) %s \n !!!!%x!!! \n", PSOCK_DATALEN(&(pConnectionState->sin)), readBytes, pConnectionState->inputBuf, (unsigned int)( pConnectionState->userData.postData + readBytes) );
            memcpy(pConnectionState->userData.postData + readBytes, pConnectionState->inputBuf, PSOCK_DATALEN(&(pConnectionState->sin)));

            LOG_DBG("read POST bytes before %d \n", readBytes);
            readBytes += PSOCK_DATALEN(&(pConnectionState->sin));
            LOG_DBG("read POST bytes %d PSOCK DATALEN %d\n", readBytes, PSOCK_DATALEN(&(pConnectionState->sin)));
            //LOG_DBG("read %s\n", pConnectionState->inputBuf);

          }
          while( readBytes < pConnectionState->userData.postDataSize );

          LOG_DBG("After While ###%x### \n", (unsigned int)pConnectionState->userData.postData);

          pConnectionState->userData.postData[readBytes++] = 0;

          pConnectionState->connBuffUsedSize += readBytes;

          LOG_DBG("PostData=>%s\n\n\n",pConnectionState->userData.postData);

          LOG_DBG("Conn Buff Used Size %d\n",pConnectionState->connBuffUsedSize);

          //printBuffer(pConnectionState->connBuffer,pConnectionState->connBuffUsedSize,CHARACTER);
        }
        else
        {
          error = MEMORY_ALLOC_ERR;
        }
      }

      if ( error == NO_ERROR )
      {
        if ( serviceCallback )
        {
          serviceCallback(pConnectionState);
        }
      }
      else
      {
        LOG_ERR("Error:%d\n",error);
        if ( error == MEMORY_ALLOC_ERR /*Other overloading related errors can be added here*/ )
        {
          //due to overloading, no memory available
          http_server_set_http_status(pConnectionState, SERVER_ERROR_SERVICE_UNAVAILABLE);
        }
        else
        {
          //some other error which can not be figured out.
          http_server_set_http_status(pConnectionState, SERVER_ERROR_INTERNAL);
        }
      }

      pConnectionState->state = STATE_OUTPUT;
    }
  }

  PSOCK_END(&(pConnectionState->sin));
}

static
PT_THREAD( sendData( ConnectionState_t* pConnectionState ) )
{
  PSOCK_BEGIN(&(pConnectionState->sout));

  LOG_DBG("Before Sending Data : \n");

  static const char* p = NULL;
  static uint8_t nIndex = 0;
  char statusCode[6];

  psock_buffered_string_send_begin(&(pConnectionState->sout));
  p = httpv1_1;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
  p = spaceString;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
  snprintf(statusCode,5,"%d ",pConnectionState->response.statusCode);
  p = statusCode;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
  p = pConnectionState->response.statusString;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
  p = lineEnd;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );

  LOG_DBG("Num Of Headers:%d\n",pConnectionState->response.numOfResponseHeaders);

  for ( nIndex=0;nIndex<pConnectionState->response.numOfResponseHeaders;nIndex++ )
  {
    p = pConnectionState->response.headers[nIndex].name;
    PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
    p = headerDelimiter;
    PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
    p = pConnectionState->response.headers[nIndex].value;
    PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
    p = lineEnd;
    PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );
  }

  p = lineEnd;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , false );

  p = pConnectionState->response.responseBuffer;
  PSOCK_BUFFERED_STRING_SEND( &(pConnectionState->sout), &p , true ); //true -> force

  PSOCK_END(&(pConnectionState->sout));
}

static PT_THREAD(handleResponse(ConnectionState_t* pConnectionState))
{
  PT_BEGIN(&(pConnectionState->outputpt));

  LOG_DBG("handle_output ->\n");
  LOG_INFO("hoSTART rt %hu\n", RTIMER_NOW());
  //leds_on(LEDS_BLUE);

  //free memory if allocated for post data
  if ( pConnectionState->userData.postData )
  {
    LOG_DBG("FREE POST DATA ###%x### \n", (unsigned int)pConnectionState->userData.postData);
    free( pConnectionState->userData.postData );
    pConnectionState->userData.postData = NULL;
    pConnectionState->userData.postDataSize = 0;
  }

  /*http_server_add_res_header(pConnectionState, HTTP_HEADER_NAME_CONNECTION, close, false);*/
  http_server_add_res_header(pConnectionState, HTTP_HEADER_NAME_SERVER, contiki, false);

  if ( !(pConnectionState->response.statusString) )
  {
    pConnectionState->response.statusString =
      getDefaultStatusString(pConnectionState->response.statusCode);
  }

  PT_WAIT_THREAD(&(pConnectionState->outputpt), sendData( pConnectionState ));

  LOG_INFO("hoEND rt %hu\n", RTIMER_NOW());

  LOG_INFO("<---Response\n\n\n");

  PSOCK_CLOSE(&(pConnectionState->sout));

  PT_END(&(pConnectionState->outputpt));
}

HttpMethod_t http_server_get_http_method(ConnectionState_t* pConnectionState)
{
  return pConnectionState->requestType;
}

void http_server_set_http_status(ConnectionState_t* pConnectionState, StatusCode_t status)
{
  pConnectionState->response.statusCode = status;
}

static void handleConnection(ConnectionState_t* pConnectionState)
{
  if(pConnectionState->state == STATE_WAITING)
  {
    handleRequest(pConnectionState);
    //int index = 0;
    //while (index++ < 1000)
      //LOG_INFO("after hr Clock Secs %lu Time %hu StateOutput %d\n", clock_seconds(), RTIMER_NOW(), pConnectionState->state);
  }

  if(pConnectionState->state == STATE_OUTPUT)
  {
    LOG_INFO("bf ho rt %hu uip_flags 0x%02x\n", RTIMER_NOW(), uip_flags);
    handleResponse(pConnectionState);
  }
}

void http_server_get_base_url(char* pcOut)
{
#if UIP_CONF_IPV6
  //DY : hack - uip_gethostaddr does not compile under IPv6, should check later, maybe because of link local and global addr stuff.
  uip_ipaddr_t* addr = &(uip_netif_physical_if.addresses[0].ipaddr);
  sprintf( pcOut,"[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d",
          ((u8_t *)&addr)[0], ((u8_t *)&addr)[1], ((u8_t *)&addr)[2], ((u8_t *)&addr)[3],
          ((u8_t *)&addr)[4], ((u8_t *)&addr)[5], ((u8_t *)&addr)[6], ((u8_t *)&addr)[7],
          ((u8_t *)&addr)[8], ((u8_t *)&addr)[9], ((u8_t *)&addr)[10], ((u8_t *)&addr)[11],
          ((u8_t *)&addr)[12], ((u8_t *)&addr)[13], ((u8_t *)&addr)[14], ((u8_t *)&addr)[15], PORT);
#else /* UIP_CONF_IPV6 */
  uip_ipaddr_t addr;
  uip_gethostaddr(&addr);
  sprintf( pcOut, "http://%d.%d.%d.%d:%d", uip_ipaddr_to_quad(&addr),PORT);
#endif /* UIP_CONF_IPV6 */
}

Error_t http_server_copy_to_response(ConnectionState_t* pConnectionState, const char* pcBuffer, uint16_t nSize)
{
  Error_t error = NO_ERROR;
  if ( nSize <= RESPONSE_BUFFER_SIZE )
  {
    memcpy(pConnectionState->response.responseBuffer, pcBuffer, nSize);
    pConnectionState->response.responseBuffIndex = nSize;
  }
  else
  {
    error = MEMORY_BOUNDARY_EXCEEDED;
  }

  return error;
}

Error_t http_server_concatenate_str_to_response(ConnectionState_t* pConnectionState, const char* pcBuffer)
{
  Error_t error = NO_ERROR;
  uint16_t nStringSize = strlen(pcBuffer);

  if ( pConnectionState->response.responseBuffIndex + nStringSize <= RESPONSE_BUFFER_SIZE )
  {
    char* pResponseBuffer = pConnectionState->response.responseBuffer + pConnectionState->response.responseBuffIndex;
    strcpy(pResponseBuffer, pcBuffer);
    pConnectionState->response.responseBuffIndex += nStringSize;
  }
  else
  {
    error = MEMORY_BOUNDARY_EXCEEDED;
  }

  return error;
}

const char* http_server_get_post_data(ConnectionState_t* pConnectionState)
{
  return pConnectionState->userData.postData;
}

const char* http_server_get_relative_url(ConnectionState_t* pConnectionState)
{
  return pConnectionState->resourceUrl;
}

#ifdef HTTP_ENERGY_EVAL

#include "sys/compower.h"

struct energy_estimate_t {
  unsigned long cpu, lpm, transmit, listen, idle_transmit, idle_listen;
}energy_estimate;

void start_energy_estimation(struct energy_estimate_t* energy_estimate, char* func_name)
{
  //get current values
  energy_estimate->cpu = energest_type_time(ENERGEST_TYPE_CPU);
  energy_estimate->lpm = energest_type_time(ENERGEST_TYPE_LPM);
  energy_estimate->transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  energy_estimate->listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  energy_estimate->idle_transmit = compower_idle_activity.transmit;
  energy_estimate->idle_listen = compower_idle_activity.listen;
}

void end_energy_estimation(struct energy_estimate_t* energy_estimate, char* func_name)
{
  static int number = 1;

  unsigned long current_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  unsigned long current_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  unsigned long current_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  unsigned long current_listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  unsigned long current_idle_transmit = compower_idle_activity.transmit;
  unsigned long current_idle_listen = compower_idle_activity.listen;

  unsigned long cpu_diff = current_cpu-energy_estimate->cpu;
  unsigned long lpm_diff = current_lpm-energy_estimate->lpm;

  printf("#Est:%d;", number++);
  printf("CPU=%ld;LPM=%ld;TCPU=%ld;TRN=%ld;LST=%ld;ID_TRN=%ld;ID_LST=%ld \n\n",
    cpu_diff,
    lpm_diff,
    cpu_diff + lpm_diff,
    current_transmit-energy_estimate->transmit,
    current_listen-energy_estimate->listen,
    current_idle_transmit-energy_estimate->idle_transmit,
    current_idle_listen-energy_estimate->idle_listen );
}
#endif //HTTP_ENERGY_EVAL

PROCESS(httpdProcess, "Httpd Process");

PROCESS_THREAD(httpdProcess, ev, data)
{
  PROCESS_BEGIN();

  #ifdef CONTIKI_TARGET_SKY
    LOG_DBG("##RF CHANNEL : %d##\n",RF_CHANNEL);
  #endif //CONTIKI_TARGET_SKY

  tcp_listen(HTONS(PORT));

  http_server_handle_req_header(HTTP_HEADER_NAME_CONTENT_LENGTH);
  http_server_handle_req_header(HTTP_HEADER_NAME_CONTENT_TYPE);
  http_server_handle_req_header(HTTP_HEADER_NAME_IF_NONE_MATCH);

  /*
   * We loop for ever, accepting new connections.
   */
  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

    ConnectionState_t *pConnState = (ConnectionState_t *)data;

    if(uip_connected())
    {
      LOG_DBG("##Connected##\n");

      pConnState = (ConnectionState_t *)malloc(sizeof(ConnectionState_t));
      if ( pConnState )
      {
        tcp_markconn(uip_conn, pConnState);

        //initialize connection state
        http_common_init_connection(pConnState);

        //-1 is needed to be able to null terminate the strings in the buffer, especially good for debugging (to have null terminated strings)
        PSOCK_INIT(&(pConnState->sin), (uint8_t*)pConnState->inputBuf, sizeof(pConnState->inputBuf)-1);
        PSOCK_INIT(&(pConnState->sout), (uint8_t*)pConnState->inputBuf, sizeof(pConnState->inputBuf)-1);
        PT_INIT(&(pConnState->outputpt));

        handleConnection(pConnState);
      }
      else
      {
        LOG_ERR("Memory Alloc Error. Aborting!\n");
        printf("EE\n");
        uip_abort();
      }

#ifdef CONTIKI_TARGET_SKY
      //xmac off - keep radio on
      //rime_mac->off(1);
#endif //CONTIKI_TARGET_SKY

#ifdef HTTP_ENERGY_EVAL
      start_energy_estimation(&energy_estimate,"energy_estimate_process");
#endif //HTTP_ENERGY_EVAL

    }
    else if(uip_aborted() || uip_closed() || uip_timedout())
    {
      if ( pConnState )
      {
        free(pConnState);
        //Following 2 lines are needed since this part of code is somehow executed twice so it tries to free the same region twice.
        //Potential bug in uip
        pConnState=NULL;
        tcp_markconn(uip_conn, pConnState);
      }
#ifdef CONTIKI_TARGET_SKY
      //xmac on
      //DY : FIX_ME : need to add here a control like if not gateway
      //rime_mac->on();
#endif //CONTIKI_TARGET_SKY
#ifdef HTTP_ENERGY_EVAL
      end_energy_estimation(&energy_estimate,"energy_estimate_process");
#endif //HTTP_ENERGY_EVAL
    }
    else //uip_poll??
    {
      handleConnection(pConnState);
    }
  }

  PROCESS_END();
}
