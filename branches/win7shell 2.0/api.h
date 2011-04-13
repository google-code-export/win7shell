#ifndef NULLSOFT_API_H
#define NULLSOFT_API_H

#include <api/service/api_service.h>
extern api_service *serviceManager;
#define WASABI_API_SVC serviceManager

#include <api/service/waServiceFactory.h>

#include "../Agave/AlbumArt/api_albumart.h"

#include <api/service/svcs/svc_imgload.h>
#include <api/service/svcs/svc_imgwrite.h>

#include <api/memmgr/api_memmgr.h>
extern api_memmgr *memmgrApi;
#define WASABI_API_MEMMGR memmgrApi

#include "../playlist/api_playlists.h"
extern api_playlists *playlistsApi;
#define AGAVE_API_PLAYLISTS playlistsApi

#include "../Agave/Language/api_language.h"

#endif