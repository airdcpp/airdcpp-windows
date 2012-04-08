#ifndef _PLAYERS_H_
#define _PLAYERS_H_

/* Some window messsages for Windows Media Player 9+, but in theory these could work for anything above WMP7 
					Reference: http://www.autohotkey.com/forum/topic8566.html								*/ 

#define WMP_PLAY 18808			// Play/Pause Track (toggle)
#define WMP_STOP 18809			// Stop 
#define WMP_PREV 18810			// Previous Track
#define WMP_NEXT 18811			// Next track 
#define WMP_REV 18812			// Rewind (Only on video?)
#define WMP_FFOR 18813			// Fast Forward (toggle)
#define WMP_VOLUP 18815			// Volume: Up
#define WMP_VOLDOWN 18816		// Volume: Down
#define WMP_MUTE 18817			// Volume: Mute (toggle)
#define WMP_SHUF 18842			// Shuffle (toggle)
#define WMP_REP 18843			// Repeat (toggle)

#define WMP_OPEN 32861			// Open "File Open" dialog
#define WMP_OPT 32825			// Open "Options" dialog
#define WMP_HELP 32826			// Open "Windows Media Player Help"

// Alternate window messages for some of the above player actions

#define WMP_ALT_PLAY 32808		// Play/Pause Track (toggle)
#define WMP_ALT_STOP 32809		// Stop 
#define WMP_ALT_PREV 32810		// Previous Track
#define WMP_ALT_NEXT 32811		// Next track 
#define WMP_ALT_FFOR 32813		// Fast Forward (toggle)
#define WMP_ALT_VOLUP 32815		// Volume: Up
#define WMP_ALT_VOLDOWN 32816	// Volume: Down
#define WMP_ALT_MUTE 32817		// Volume: Mute (toggle)
#define WMP_ALT_SHUF 32842		// Shuffle (toggle)
#define WMP_ALT_REP 32843		// Repeat (toggle)

/* [Untested] Some window messsages for MediaPlayer Classic (mplayerc). Reference: MPC Options > Keys */

#define MPC_PLAY_TOG 889		// Play/Pause (toggle)
#define MPC_PLAY 887			// Play
#define MPC_PAUSE 888			// Pause
#define MPC_STOP 890			// Stop 
#define MPC_PREV 920			// Previous
#define MPC_NEXT 921			// Next
#define MPC_VOLUP 907			// Volume: Up
#define MPC_VOLDOWN 908			// Volume: Down
#define MPC_MUTE 909			// Volume: Mute

#define MPC_OPEN 800			// Open "File Open" dialog
#define MPC_OPT 886				// Open "Options" dialog

/* Spotify */

#define S_PLAY 0xd0000L			// Play
#define S_PAUSE 0xd0000L		// Pause
#define S_STOP 0xd0000L			// Stop 
#define S_PREV 0xc0000L			// Previous
#define S_NEXT 0xb0000L			// Next
#define S_VOLUP 0xa0000L		// Volume: Up
#define S_VOLDOWN 0x90000L		// Volume: Down
#define S_MUTE 0x80000L			// Volume: Mute


#include <string>

namespace dcpp {

class Players {
public:
	static string getWMPSpam(HWND playerWnd = NULL);
	static string getSpotifySpam(HWND playerWnd = NULL);
	static string getItunesSpam(HWND playerWnd = NULL);
	static string getMPCSpam();
	static string getWinAmpSpam();
};

}

#endif