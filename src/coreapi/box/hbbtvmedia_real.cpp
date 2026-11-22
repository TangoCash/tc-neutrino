/*
 * hbbtvmedia_real.cpp - the media of an HbbTV application, on the box's decoders
 *
 * Copyright (C) 2026 NI-Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include "coreapi/base/deps.h"

#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

#include <hardware/playback.h>
#include <hardware/video.h>
#include <zapit/client/zapitclient.h>

extern cVideo *videoDecoder;
extern CZapitClient *g_Zapit;

namespace coreapi
{

namespace
{

/* A player without a screen of its own, like the movie player underneath:
   live television is stopped and locked while it plays, and given back when
   it stops. */
class RealHbbtvMedia : public HbbtvMedia
{
	public:
		Status play(const std::string &url, int start_ms)
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopLocked();
			printf("[hbbtv-media] play %s from %d ms\n", url.c_str(), start_ms);

			if (!live_locked)
			{
				g_Zapit->lockPlayBack();
				live_locked = true;
			}
			/* stopping live television blanked the picture */
			if (videoDecoder->getBlank())
				videoDecoder->setBlank(false);

			playback = new cPlayback(0);
			playback->Open(PLAYMODE_FILE);
			state = "starting";
			start_result = 0;
			start_position = start_ms;
			/* opening a network stream can take seconds; nothing is held
			   meanwhile, the next call picks the result up */
			cPlayback *pb = playback;
			start_thread = std::thread([this, pb, url]() { start_result = pb->Start(url) ? 1 : -1; });
			return Status::Ok;
		}

		void pause()
		{
			std::lock_guard<std::mutex> lock(mutex);
			finishStart();
			if (playback && state == "playing")
			{
				playback->SetSpeed(0);
				state = "paused";
			}
		}

		void resume()
		{
			std::lock_guard<std::mutex> lock(mutex);
			finishStart();
			if (playback && state == "paused")
			{
				playback->SetSpeed(1);
				state = "playing";
			}
		}

		void seek(int position_ms)
		{
			std::lock_guard<std::mutex> lock(mutex);
			finishStart();
			if (!playback)
				return;
			if (state == "playing" || state == "paused")
				playback->SetPosition(position_ms, true);
			else if (state == "starting")
				start_position = position_ms;
		}

		void stop()
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopLocked();
		}

		void window(int x, int y, int width, int height, int page_width, int page_height)
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (!playback || page_width <= 0 || page_height <= 0)
				return;
			/* nothing to show, or (almost) the whole page: the full screen */
			if (width <= 0 || height <= 0 || (width >= page_width - 2 && height >= page_height - 2))
				videoDecoder->Pig(-1, -1, -1, -1);
			else
				videoDecoder->Pig(x, y, width, height, page_width, page_height);
		}

		HbbtvMediaState status()
		{
			std::lock_guard<std::mutex> lock(mutex);
			finishStart();
			HbbtvMediaState out;
			out.position_ms = 0;
			out.duration_ms = 0;
			if (playback && (state == "playing" || state == "paused"))
			{
				if (!playback->GetPosition(out.position_ms, out.duration_ms) || !playback->IsPlaying())
					state = "ended";
			}
			out.state = state;
			return out;
		}

	private:
		/* the start thread has finished: start or report the failure */
		void finishStart()
		{
			if (state != "starting" || start_result == 0)
				return;
			if (start_thread.joinable())
				start_thread.join();
			if (start_result > 0)
			{
				playback->SetSpeed(1);
				if (start_position > 0)
					playback->SetPosition(start_position, true);
				state = "playing";
			}
			else
				state = "error";
		}

		void stopLocked()
		{
			if (start_thread.joinable())
			{
				if (start_result == 0 && playback)
					playback->RequestAbort();
				start_thread.join();
			}
			if (playback)
			{
				/* libeplayer3 does not stop while paused */
				if (state == "paused")
					playback->SetSpeed(1);
				playback->Close();
				delete playback;
				playback = NULL;
				printf("[hbbtv-media] stopped\n");
			}
			if (live_locked)
			{
				videoDecoder->Pig(-1, -1, -1, -1);
				g_Zapit->unlockPlayBack();
				live_locked = false;
			}
			state = "idle";
		}

		std::mutex mutex;
		cPlayback *playback = NULL;
		std::thread start_thread;
		std::atomic<int> start_result { 0 };
		int start_position = 0;
		std::string state = "idle";
		bool live_locked = false;
};

RealHbbtvMedia g_real_hbbtv_media;

} // namespace

void installRealHbbtvMedia() { setHbbtvMedia(&g_real_hbbtv_media); }

} // namespace coreapi
