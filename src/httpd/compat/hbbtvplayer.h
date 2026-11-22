#ifndef HTTPD_COMPAT_HBBTVPLAYER_H
#define HTTPD_COMPAT_HBBTVPLAYER_H

#include <mutex>
#include <string>
#include <thread>

class cPlayback;

/* A player without a screen of its own, for the media of an HbbTV
   application: the browser draws the page, this plays the stream on the
   decoders of neutrino below it, and the browser drives it through the
   control API (zapto?hbbtv...). In this process, not in the browser's: the
   GL driver crashes when one process uses it and a decoder at once. */
class CHbbtvPlayer
{
public:
	static CHbbtvPlayer &instance();

	bool play(const std::string &url, int startMs);
	void pause();
	void resume();
	void seek(int positionMs);
	void stop();
	/* where the page shows the video, in page pixels */
	void window(int x, int y, int width, int height, int pageWidth, int pageHeight);
	/* "<state> <position ms> <duration ms>"; state is one of idle,
	   starting, playing, paused, ended, error */
	std::string status();

private:
	CHbbtvPlayer();
	void stopLocked();
	void starter(cPlayback *playback, std::string url, int startMs, unsigned int generation);

	std::mutex mutex;
	cPlayback *playback;
	std::thread startThread;
	std::string state;
	unsigned int generation;
	bool liveLocked;
};

#endif
