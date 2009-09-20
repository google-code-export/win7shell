/*

2009 Killboy/SND

---------------------------

This is a set of functions to work with tab controls.
They should be used by themselves, that is without modifying tab items yourself
as the functions assume certain conditions and might cause undefined behaviour
if things aren't as expected.

Some of the things you shouldn't do:

- Add a tab without using the appropiate function
- Remove a tab without using the appropiate function
- Change the tab items' lParam value (they hold the window handles)
- Change the tab's ImageList
- Call SetProp on the tab window with "TabCtrlCurWnd" ;D

You can still modify the tab control's GWLP_USERDATA.

In order for the tab selection to work, you will have to add this piece of
code to the tab control's parent window handler (usually the dialog procedure):

<-- CODE -->

case WM_NOTIFY:
	switch(((NMHDR *)lParam)->code)
	{
	case TCN_SELCHANGE: // Get currently selected tab window to front
		TabToFront(TabWindow, -1);
		break;
	default:
		return false;
	}
	break;

<-- /CODE -->

The implementation might depend on how your code handles the message param, usually it would be like this:

<-- CODE -->

INT_PTR CALLBACK DialogProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case SOME_MESSAGE:
			break;
		// more messages
		case WM_NOTIFY:
			// Insert code above here
			break;
		default:
			return false;
	}

	return true;
}

<-- /CODE -->

*/

#ifndef TABS_H
#define TABS_H

#include <windows.h>

/*
int AddTab(HWND TabWindow, HWND Window, char * Caption, int Index)

Adds a tab item to the tab control and associates the window to it.
The window is moved and resized to fit the tab control dimensions.

TabWindow - Window handle of the tab control
Window    - Window handle of the window to associate with the tab item
Caption   - C-String that contains the tab item's caption
Index     - Index of the new item (indices start with 0)
            if Index is -1, it adds the tab item at the end

Returns the new index or -1 if an error occured.
*/

int AddTab(HWND TabWindow, HWND Window, wchar_t * Caption, int Index);

/*
bool RemoveTab(HWND TabWindow, int Index)

Removes the tab item from the tab control.
The associated icon (if any) is removed from the image list.
If it is the currently selected tab item it selects the next one.

TabWindow - Window handle of the tab control
Index     - Index of the item to remove

Returns true if no error occured.
*/

bool RemoveTab(HWND TabWindow, int Index);

/*
bool TabCleanup(HWND TabWindow)

Removes all tab items from the tab control.
Cleans up internal resources and hides all windows associated with it.

It's recommended to call this function before the tab control is destroyed,
e.g. if you get a WM_CLOSE message in your dialogbox proc.

TabWindow - Window handle of the tab control

Returns true if no error occured.
*/

bool TabCleanup(HWND TabWindow);

/*
bool SetTabIcon(HWND TabWindow, int Index, HICON Icon)

Sets the tab item's icon.
Supports 32bit icons (24bit + 8bit alpha) if comctl32.dll v6 is used (ie. visual styles enabled).
The icon should be 16x16 in size and at least contain an 8bit (256 colors) channel.

TabWindow - Window handle of the tab control
Index     - Index of the item to set the icon for
Icon      - The icon to set for the tab item
            If Icon is 0, it removes the current icon

Returns true if no error occured.
*/

bool SetTabIcon(HWND TabWindow, int Index, HICON Icon);

/*
int TabToFront(HWND TabWindow, int Index)

Selects the specified tab and shows the appropiate window.

TabWindow - Window handle of the tab control
Index     - Index of the item to select
            if Index is -1, it show's the currently selected tab's window

Returns the selected tab item index or -1 if an error occured.
*/

int TabToFront(HWND TabWindow, int Index);

#endif
