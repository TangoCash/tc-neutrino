#include "httpd/compat/hbbtvplayer.h"

#include <atomic>
#include <cstdio>

#include <global.h>
#include <hardware/playback.h>
#include <hardware/video.h>
#include <zapit/client/zapitclient.h>

extern cVideo *videoDecoder;

/* set by the start thread once cPlayback::Start returned: 1 ok, -1 failed */
static std::atomic<int> startResult(0);
static int startPosition = 0;

CHbbtvPlayer &CHbbtvPlayer::instance()
{
	static CHbbtvPlayer player;
	return player;
}

CHbbtvPlayer::CHbbtvPlayer()
	: playback(NULL), state("idle"), generation(0), liveLocked(false)
{
}

void CHbbtvPlayer::starter(cPlayback *pb, std::string url, int, unsigned int)
{
	/* opening a network stream can take seconds; no lock held here, the
	   result is picked up by the next call of the browser */
	startResult = pb->Start(url) ? 1 : -1;
}

/* the start thread has finished: start or report the failure */
static void finishStart(cPlayback *playback, std::thread &startThread, std::string &state)
{
	if (state != "starting" || startResult == 0)
		return;
	if (startThread.joinable())
		startThread.join();
	if (startResult > 0) {
		playback->SetSpeed(1);
		if (startPosition > 0)
			playback->SetPosition(startPosition, true);
		state = "playing";
	} else {
		state = "error";
	}
}

bool CHbbtvPlayer::play(const std::string &url, int startMs)
{
	std::lock_guard<std::mutex> lock(mutex);
	stopLocked();
	printf("[hbbtvplayer] play %s from %d ms\n", url.c_str(), startMs);

	/* like the movie player: stop live TV and keep zapit from restarting it */
	if (!liveLocked) {
		g_Zapit->lockPlayBack();
		liveLocked = true;
	}
	/* stopping live TV blanked the picture */
	if (videoDecoder->getBlank())
		videoDecoder->setBlank(false);

	playback = new cPlayback(0);
	playback->Open(PLAYMODE_FILE);
	state = "starting";
	startResult = 0;
	startPosition = startMs;
	++generation;
	startThread = std::thread(&CHbbtvPlayer::starter, this, playback, url, startMs, generation);
	return true;
}

void CHbbtvPlayer::pause()
{
	std::lock_guard<std::mutex> lock(mutex);
	finishStart(playback, startThread, state);
	if (playback && state == "playing") {
		playback->SetSpeed(0);
		state = "paused";
	}
}

void CHbbtvPlayer::resume()
{
	std::lock_guard<std::mutex> lock(mutex);
	finishStart(playback, startThread, state);
	if (playback && state == "paused") {
		playback->SetSpeed(1);
		state = "playing";
	}
}

void CHbbtvPlayer::seek(int positionMs)
{
	std::lock_guard<std::mutex> lock(mutex);
	finishStart(playback, startThread, state);
	if (!playback)
		return;
	if (state == "playing" || state == "paused")
		playback->SetPosition(positionMs, true);
	else if (state == "starting")
		startPosition = positionMs;
}

void CHbbtvPlayer::stop()
{
	std::lock_guard<std::mutex> lock(mutex);
	stopLocked();
}

void CHbbtvPlayer::stopLocked()
{
	if (startThread.joinable()) {
		if (startResult == 0 && playback)
			playback->RequestAbort();
		startThread.join();
	}
	if (playback) {
		/* libeplayer3 does not stop while paused */
		if (state == "paused")
			playback->SetSpeed(1);
		playback->Close();
		delete playback;
		playback = NULL;
		printf("[hbbtvplayer] stopped\n");
	}
	if (liveLocked) {
		videoDecoder->Pig(-1, -1, -1, -1);
		g_Zapit->unlockPlayBack();
		liveLocked = false;
	}
	state = "idle";
}

void CHbbtvPlayer::window(int x, int y, int width, int height, int pageWidth, int pageHeight)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (!playback || pageWidth <= 0 || pageHeight <= 0)
		return;
	/* nothing to show or (almost) the whole page: the full screen */
	if (width <= 0 || height <= 0 || (width >= pageWidth - 2 && height >= pageHeight - 2))
		videoDecoder->Pig(-1, -1, -1, -1);
	else
		videoDecoder->Pig(x, y, width, height, pageWidth, pageHeight);
}

std::string CHbbtvPlayer::status()
{
	std::lock_guard<std::mutex> lock(mutex);
	finishStart(playback, startThread, state);
	int position = 0, duration = 0;
	if (playback && (state == "playing" || state == "paused")) {
		if (!playback->GetPosition(position, duration) || !playback->IsPlaying())
			state = "ended";
	}
	char buf[64];
	snprintf(buf, sizeof(buf), "%s %d %d", state.c_str(), position, duration);
	return buf;
}
