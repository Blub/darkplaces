enum
{
	CURLCBSTATUS_OK = 0,
	CURLCBSTATUS_FAILED = -1, // failed for generic reason (e.g. buffer too small)
	CURLCBSTATUS_ABORTED = -2, // aborted by curl --cancel
	CURLCBSTATUS_SERVERERROR = -3, // only used if no HTTP status code is available
	CURLCBSTATUS_UNKNOWN = -4 // should never happen
};
typedef void (*curl_callback_t) (int status, size_t length_received, unsigned char *buffer, void *cbdata);
// code is one of the CURLCBSTATUS constants, or the HTTP error code (when > 0).

void Curl_Run(void);
qboolean Curl_Running(void);
qboolean Curl_Begin_ToFile(const char *URL, const char *name, qboolean ispak, qboolean forthismap);
qboolean Curl_Begin_ToMemory(const char *URL, unsigned char *buf, size_t bufsize, curl_callback_t callback, void *cbdata);
	// NOTE: if it returns false, the callback will NOT get called, so free your buffer then!
void Curl_Cancel_ToMemory(curl_callback_t callback, void *cbdata);
	// removes all downloads with the given callback and cbdata (this does NOT call the callbacks!)
void Curl_Init(void);
void Curl_Init_Commands(void);
void Curl_Shutdown(void);
void Curl_CancelAll(void);
void Curl_Clear_forthismap(void);
qboolean Curl_Have_forthismap(void);
void Curl_Register_predownload(void);

void Curl_ClearRequirements(void);
void Curl_RequireFile(const char *filename);
void Curl_SendRequirements(void);

typedef struct Curl_downloadinfo_s
{
	char filename[MAX_QPATH];
	double progress;
	double speed;
	qboolean queued;
}
Curl_downloadinfo_t;
Curl_downloadinfo_t *Curl_GetDownloadInfo(int *nDownloads, const char **additional_info);
	// this may and should be Z_Free()ed
	// the result is actually an array
	// an additional info string may be returned in additional_info as a
	// pointer to a static string (but the argument may be NULL if the caller
	// does not care)
