This branch is a development version to try to resolve some of the issues with the plug-in and make it Winamp 5.5+ compatible. To build this version, it assumes that the source is located in the following manner:

<Winamp 5.55 SDK folder>
-<gen_win7shell>
-<sdk>

As well in this branch is a copy of api_playlists.h which needs to be placed into sdk\playlist so it can use this wasabi service to query the installed media library playlists.

In the project settings, 'Release New' is the active project as i've not been able to get a working version of the plug-in to compile when used with anything other than the debug crt (crashes in the cache map due to never leaving after the first call when adding metadata to the cache for some reason with the release mode version).

DrO