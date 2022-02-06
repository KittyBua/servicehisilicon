#ifndef __serviceHisilicon_h
#define __serviceHisilicon_h

#include <lib/base/message.h>
#include <lib/service/iservice.h>
#include <lib/dvb/pmt.h>
#include <lib/dvb/subtitle.h>
#include <lib/dvb/teletext.h>
/* for subtitles */
#include <lib/gui/esubtitle.h>

#include "hisilicon.h"

class eStaticServiceHisiliconInfo;

class eServiceFactoryHisilicon: public iServiceHandler
{
	DECLARE_REF(eServiceFactoryHisilicon);
public:
	eServiceFactoryHisilicon();
	virtual ~eServiceFactoryHisilicon();
	enum { id = 0x1001 };

		// iServiceHandler
	RESULT play(const eServiceReference &, ePtr<iPlayableService> &ptr);
	RESULT record(const eServiceReference &, ePtr<iRecordableService> &ptr);
	RESULT list(const eServiceReference &, ePtr<iListableService> &ptr);
	RESULT info(const eServiceReference &, ePtr<iStaticServiceInformation> &ptr);
	RESULT offlineOperations(const eServiceReference &, ePtr<iServiceOfflineOperations> &ptr);
private:
	ePtr<eStaticServiceHisiliconInfo> m_service_info;
};

class eStaticServiceHisiliconInfo: public iStaticServiceInformation
{
	DECLARE_REF(eStaticServiceHisiliconInfo);
	friend class eServiceFactoryHisilicon;
	eStaticServiceHisiliconInfo();
public:
	RESULT getName(const eServiceReference &ref, std::string &name);
	int getLength(const eServiceReference &ref);
	int getInfo(const eServiceReference &ref, int w);
	int isPlayable(const eServiceReference &ref, const eServiceReference &ignore, bool simulate) { return 1; }
	long long getFileSize(const eServiceReference &ref);
	RESULT getEvent(const eServiceReference &ref, ePtr<eServiceEvent> &ptr, time_t start_time);
};

class eHisiliconBufferInfo: public iStreamBufferInfo
{
	DECLARE_REF(eHisiliconBufferInfo);
	int bufferPercentage;
	int inputRate;
	int outputRate;
	int bufferSpace;
	int bufferSize;

public:
	eHisiliconBufferInfo(int percentage, int inputrate, int outputrate, int space, int size);

	int getBufferPercentage() const;
	int getAverageInputRate() const;
	int getAverageOutputRate() const;
	int getBufferSpace() const;
	int getBufferSize() const;
};

class eServiceHisiliconInfoContainer: public iServiceInfoContainer
{
	DECLARE_REF(eServiceHisiliconInfoContainer);

	double doubleValue;

	unsigned char *bufferData;
	unsigned int bufferSize;

public:
	eServiceHisiliconInfoContainer();
	~eServiceHisiliconInfoContainer();

	double getDouble(unsigned int index) const;
	unsigned char *getBuffer(unsigned int &size) const;
	void setDouble(double value);
};

class eServiceHisilicon: public iPlayableService, public iPauseableService,
	public iServiceInformation, public iSeekableService, public iAudioTrackSelection, public iAudioChannelSelection,
	public iSubtitleOutput, public iStreamedService, public iAudioDelay, public sigc::trackable, public iCueSheet
{
	DECLARE_REF(eServiceHisilicon);
public:
	virtual ~eServiceHisilicon();

		// iPlayableService
	RESULT connectEvent(const sigc::slot<void(iPlayableService*,int)> &event, ePtr<eConnection> &connection);
	RESULT start();
	RESULT stop();

	RESULT pause(ePtr<iPauseableService> &ptr);
	RESULT setSlowMotion(int ratio);
	RESULT setFastForward(int ratio);

	RESULT seek(ePtr<iSeekableService> &ptr);
	RESULT audioTracks(ePtr<iAudioTrackSelection> &ptr);
	RESULT audioChannel(ePtr<iAudioChannelSelection> &ptr);
	RESULT subtitle(ePtr<iSubtitleOutput> &ptr);
	RESULT audioDelay(ePtr<iAudioDelay> &ptr);
	RESULT cueSheet(ePtr<iCueSheet> &ptr);

		// not implemented (yet)
	RESULT setTarget(int target, bool noaudio = false) { return -1; }
	RESULT frontendInfo(ePtr<iFrontendInformation> &ptr) { ptr = 0; return -1; }
	RESULT subServices(ePtr<iSubserviceList> &ptr) { ptr = 0; return -1; }
	RESULT timeshift(ePtr<iTimeshiftService> &ptr) { ptr = 0; return -1; }
	RESULT tap(ePtr<iTapService> &ptr) { ptr = nullptr; return -1; };
//	RESULT cueSheet(ePtr<iCueSheet> &ptr) { ptr = 0; return -1; }

		// iCueSheet
	PyObject *getCutList();
	void setCutList(SWIG_PYOBJECT(ePyObject));
	void setCutListEnable(int enable);

	RESULT rdsDecoder(ePtr<iRdsDecoder> &ptr) { ptr = 0; return -1; }
	RESULT keys(ePtr<iServiceKeys> &ptr) { ptr = 0; return -1; }
	RESULT stream(ePtr<iStreamableService> &ptr) { ptr = 0; return -1; }

	void setQpipMode(bool value, bool audio) { }

		// iPausableService
	RESULT pause();
	RESULT unpause();

	RESULT info(ePtr<iServiceInformation>&);

		// iSeekableService
	RESULT getLength(pts_t &SWIG_OUTPUT);
	RESULT seekTo(pts_t to);
	RESULT seekRelative(int direction, pts_t to);
	RESULT getPlayPosition(pts_t &SWIG_OUTPUT);
	RESULT setTrickmode(int trick);
	RESULT isCurrentlySeekable();

		// iServiceInformation
	RESULT getName(std::string &name);
	RESULT getEvent(ePtr<eServiceEvent> &evt, int nownext);
	int getInfo(int w);
	std::string getInfoString(int w);
	ePtr<iServiceInfoContainer> getInfoObject(int w);

		// iAudioTrackSelection
	int getNumberOfTracks();
	RESULT selectTrack(unsigned int i);
	RESULT getTrackInfo(struct iAudioTrackInfo &, unsigned int n);
	int getCurrentTrack();

		// iAudioChannelSelection
	int getCurrentChannel();
	RESULT selectChannel(int i);

		// iSubtitleOutput
	RESULT enableSubtitles(iSubtitleUser *user, SubtitleTrack &track);
	RESULT disableSubtitles();
	RESULT getSubtitleList(std::vector<SubtitleTrack> &sublist);
	RESULT getCachedSubtitle(SubtitleTrack &track);

		// iStreamedService
	RESULT streamed(ePtr<iStreamedService> &ptr);
	ePtr<iStreamBufferInfo> getBufferCharge();
	int setBufferSize(int size);

		// iAudioDelay
	int getAC3Delay();
	int getPCMDelay();
	void setAC3Delay(int);
	void setPCMDelay(int);

	struct audioStream
	{
		int type;
		int pid;
		std::string language_code; /* iso-639, if available. */
		audioStream()
		{
		}
	};
	struct subtitleStream
	{
		int type;
		std::string language_code; /* iso-639, if available. */
		int id;
		subtitleStream()
		{
		}
	};
	struct bufferInfo
	{
		int bufferPercent;
		int avgInRate;
		int avgOutRate;
		int64_t bufferingLeft;
		bufferInfo()
			:bufferPercent(0), avgInRate(0), avgOutRate(0), bufferingLeft(-1)
		{
		}
	};
	struct errorInfo
	{
		std::string error_message;
		std::string missing_codec;
	};

protected:
	ePtr<eTimer> m_nownext_timer;
	ePtr<eServiceEvent> m_event_now, m_event_next;
	void updateEpgCacheNowNext();

		/* cuesheet */
	struct cueEntry
	{
		pts_t where;
		unsigned int what;

		bool operator < (const struct cueEntry &o) const
		{
			return where < o.where;
		}
		cueEntry(const pts_t &where, unsigned int what) :
			where(where), what(what)
		{
		}
	};

	std::multiset<cueEntry> m_cue_entries;
	int m_cuesheet_changed, m_cutlist_enabled;
	void loadCuesheet();
	void saveCuesheet();
private:
	static int pcm_delay;
	static int ac3_delay;
	int m_currentAudioStream;
	int m_currentSubtitleStream;
	int m_cachedSubtitleStream;
	int selectAudioStream(int i);
	std::vector<audioStream> m_audioStreams;
	std::vector<subtitleStream> m_subtitleStreams;
	iSubtitleUser *m_subtitle_widget;
	friend class eServiceFactoryHisilicon;
	eServiceReference m_ref;
	bool m_paused;
	bool m_buffering;
	/* cuesheet load check */
	bool m_cuesheet_loaded;
	/* servicemHisilicon chapter TOC support CVR */
	bufferInfo m_bufferInfo;
	errorInfo m_errorInfo;
	std::string m_download_buffer_path;
	eServiceHisilicon(eServiceReference ref);
	sigc::signal<void(iPlayableService*,int)> m_event;
	enum
	{
		stIdle, stRunning, stStopped,
	};
	int m_state;
	static const char *getVidFormatStr(uint32_t format);
	static const char *getAudFormatStr(uint32_t format);
	static const char *getSubFormatStr(uint32_t format);
	int m_audio_fd, m_video_fd;
	int netlink_socket;
	struct nlmsghdr *nlh;
	int receive_netlink_message(void);
	void video_event(int);
	void netlink_event(int);
	ePtr<eSocketNotifier> m_sn, m_sn_netlink;
	int m_player_state;
	uint32_t m_seekable;
	int m_bufferpercentage;
	uint32_t m_download_progress;

	struct streamid
	{
		uint16_t programid;
		uint16_t videostreamid;
		uint16_t audiostreamid;
		uint16_t subtitlestreamid;
	} streamid;

	HI_FORMAT_FILE_INFO_S fileinfo;

	pts_t m_prev_decoder_time;
	int m_decoder_time_valid_state;

	void sourceTimeout();

	RESULT seekToImpl(pts_t to);

	int m_aspect, m_width, m_height, m_framerate, m_progressive, m_gamma;
	int m_bitrate;
	std::string m_useragent;
	std::string m_extra_headers;
};

#endif
