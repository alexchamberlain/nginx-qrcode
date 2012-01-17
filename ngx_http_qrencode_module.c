/*
 * Copyright 2012 Alex Chamberlain
 */

/* Nginx Includes */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/* GD Includes */
#include <gd.h>
#include <qrencode.h>

/**
 * Types
 */

/* Main Configuration */
typedef struct {
} ngx_http_qrencode_main_conf_t;

/* Location Configuration */
typedef struct {
} ngx_http_qrencode_loc_conf_t;

/**
 * Public Interface
 */

/* Module definition. */
// Forward definitions - variables
static ngx_http_module_t ngx_http_qrencode_module_ctx;
static ngx_command_t ngx_http_qrencode_commands[];

// Forward definitions - functions
static ngx_int_t ngx_http_qrencode_init_worker(ngx_cycle_t* cycle);

ngx_module_t ngx_http_qrencode_module = {
    NGX_MODULE_V1,
    &ngx_http_qrencode_module_ctx,
    ngx_http_qrencode_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    ngx_http_qrencode_init_worker,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

/* Module context. */
// Forward declarations - functions
static void* ngx_http_qrencode_create_main_conf(ngx_conf_t* directive);
static void* ngx_http_qrencode_create_loc_conf(ngx_conf_t* directive);
static char* ngx_http_qrencode_merge_loc_conf(ngx_conf_t* directive, void* parent, void* child);

static ngx_http_module_t ngx_http_qrencode_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */
    ngx_http_qrencode_create_main_conf,
    NULL, /* init main configuration */
    NULL, /* create server configuration */
    NULL, /* init server configuration */
    ngx_http_qrencode_create_loc_conf,
    ngx_http_qrencode_merge_loc_conf
};

/* Array specifying how to handle configuration directives. */
// Forward declarations - functions
char * ngx_http_config_qrencode(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy);

static ngx_command_t ngx_http_qrencode_commands[] = {
    {
        ngx_string("qrencode"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_http_config_qrencode,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};


static ngx_int_t ngx_http_qrencode_handler(ngx_http_request_t* request);

static ngx_int_t ngx_http_qrencode_init_worker(ngx_cycle_t* cycle) {
    return NGX_OK;
}

/* Parse the 'qrencode' directive. */
char * ngx_http_config_qrencode(ngx_conf_t *cf, ngx_command_t *cmd, void *void_conf) {
  ngx_http_core_loc_conf_t *core_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  core_conf-> handler = ngx_http_qrencode_handler;

  return NGX_CONF_OK;
}

static void *ngx_http_qrencode_create_main_conf(ngx_conf_t *cf) {
  ngx_http_qrencode_main_conf_t  *qrencode_main_conf;

  qrencode_main_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_qrencode_main_conf_t));
  if (qrencode_main_conf == NULL) {
    return NULL;
  }

  return qrencode_main_conf;
}

static void* ngx_http_qrencode_create_loc_conf(ngx_conf_t* directive) {
  ngx_http_qrencode_loc_conf_t* qrencode_conf;

  qrencode_conf = ngx_pcalloc(directive->pool, sizeof(ngx_http_qrencode_loc_conf_t));
  if (qrencode_conf == NULL) {
    ngx_conf_log_error(NGX_LOG_EMERG, directive, 0,
		       "Failed to allocate memory for GridFS Location Config.");
    return NGX_CONF_ERROR;
  }

  return qrencode_conf;
}

static char* ngx_http_qrencode_merge_loc_conf(ngx_conf_t* cf, void* void_parent, void* void_child) {
    /*ngx_http_qrencode_loc_conf_t *parent = void_parent;
    ngx_http_qrencode_loc_conf_t *child = void_child;
    ngx_http_qrencode_main_conf_t *qrencode_main_conf = ngx_http_conf_get_module_main_conf(cf, ngx_http_qrencode_module);
    ngx_http_qrencode_loc_conf_t **qrencode_loc_conf;*/

    return NGX_CONF_OK;
}

static char h_digit(char hex) {
    return (hex >= '0' && hex <= '9') ? hex - '0': ngx_tolower(hex)-'a'+10;
}

static int htoi(char* h) {
    char ok[] = "0123456789AaBbCcDdEeFf";

    if (ngx_strchr(ok, h[0]) == NULL || ngx_strchr(ok,h[1]) == NULL) { return -1; }
    return h_digit(h[0])*16 + h_digit(h[1]);
}

static int url_decode(char * filename) {
    char * read = filename;
    char * write = filename;
    char hex[3];
    int c;

    hex[2] = '\0';
    while (*read != '\0'){
        if (*read == '%') {
            hex[0] = *(++read);
            if (hex[0] == '\0') return 0;
            hex[1] = *(++read);
            if (hex[1] == '\0') return 0;
            c = htoi(hex);
            if (c == -1) return 0;
            *write = (char)c;
        }
        else *write = *read;
        read++;
        write++;
    }
    *write = '\0';
    return 1;
}

static ngx_int_t ngx_http_qrencode_handler(ngx_http_request_t* request) {
    ngx_http_qrencode_loc_conf_t* qrencode_conf;
    ngx_http_core_loc_conf_t* core_conf;
    ngx_str_t location_name;
    ngx_str_t full_uri;
    char* value;

    ngx_int_t rc = NGX_OK;

    qrencode_conf = ngx_http_get_module_loc_conf(request, ngx_http_qrencode_module);
    core_conf = ngx_http_get_module_loc_conf(request, ngx_http_core_module);

    // ---------- ENSURE MONGO CONNECTION ---------- //

    // ---------- RETRIEVE KEY ---------- //

    location_name = core_conf->name;
    full_uri = request->uri;

    //ngx_log_error(NGX_LOG_ALERT, request->connection->log, 0, "%*s", full_uri.len, full_uri.data);

    if (full_uri.len < location_name.len) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                      "Invalid location name or uri.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    value = (char*)malloc(sizeof(char) * (full_uri.len - location_name.len + 1));
    if (value == NULL) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                      "Failed to allocate memory for value buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    memcpy(value, full_uri.data + location_name.len, full_uri.len - location_name.len);
    value[full_uri.len - location_name.len] = '\0';

    if (!url_decode(value)) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                      "Malformed request.");
        free(value);
        return NGX_HTTP_BAD_REQUEST;
    }

    free(value);

  QRcode *code;
  //ngx_str_t res = ngx_string("OK");
  ngx_buf_t* buffer;
  ngx_chain_t out;

  int version = 1;
  QRecLevel level = QR_ECLEVEL_L;
  QRencodeMode hint = QR_MODE_8;
  int casesensitive = 1;
  int code_size = 4;

  int margin = 4;

  code = QRcode_encodeString("http://www.alexchamberlain.co.uk", version, level, hint, casesensitive);

  if(code == NULL) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "Failed to encode address.");
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  /*ngx_table_elt_t* modules_header = ngx_list_push(&request->headers_out.headers);
  if(modules_header == NULL) {
    return NGX_ERROR;
  }

  modules_header->hash = 1;
  modules_header->key = ngx_string("X-Modules");
  modules_A*/

  ngx_log_error(NGX_LOG_DEBUG, request->connection->log, 0, "Number of Modules: %d", code->width);

  gdImagePtr img;

  int img_size = (code->width + margin*2)*code_size;

  img = gdImageCreate(img_size, img_size);

  if(img == NULL) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "gdImageCreate() failed.");
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  int black = gdImageColorAllocate(img, 0, 0, 0);
  int white = gdImageColorAllocate(img, 255, 255, 255);
  gdImageFilledRectangle(img, 0, 0, img_size - 1, img_size - 1, white);

  int x, y;
  u_char * p;

  p = code->data;

  for(x = margin; x < code->width + margin; ++x) {
    for(y = margin; y < code->width + margin; ++y) {
      if(*p & 1) {
	gdImageFilledRectangle(img,
	                 code_size*x,
			 code_size*y,
			 code_size*(x + 1) - 1,
			 code_size*(y + 1) - 1,
			 black);
      } else {
	gdImageFilledRectangle(img,
	                 code_size*x,
			 code_size*y,
			 code_size*(x + 1) - 1,
			 code_size*(y + 1) - 1,
			 white);
      }
      ++p;
    }
  }

  int size;
  u_char * out_img = gdImagePngPtr(img, &size);

  gdImageDestroy(img);

  QRcode_free(code);

  request->headers_out.status = NGX_HTTP_OK;
  request->headers_out.content_length_n = size;
  ngx_str_set(&request->headers_out.content_type, "image/png");
  ngx_http_send_header(request);

  // ---------- SEND THE BODY ---------- //

  /* Allocate space for the response buffer */
  buffer = ngx_pcalloc(request->pool, sizeof(ngx_buf_t));
  if (buffer == NULL) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
		  "Failed to allocate response buffer");
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  /* Set up the buffer chain */
  buffer->pos = out_img;
  buffer->last = out_img + size; // Don't write NULL
  buffer->memory = 1;
  buffer->last_buf = 1;
  out.buf = buffer;
  out.next = NULL;

  /* Serve the Chunk */
  rc = ngx_http_output_filter(request, &out);

  return rc;
}
