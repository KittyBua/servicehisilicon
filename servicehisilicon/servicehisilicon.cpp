#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/base/nconfig.h>
#include <lib/base/object.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/decoder.h>
#include <lib/components/file_eraser.h>
#include <lib/gui/esubtitle.h>
#include <lib/service/service.h>
#include <lib/gdi/gpixmap.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string>
#include <sys/socket.h>
#include <linux/netlink.h>

#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>

#include "servicehisilicon.h"

#ifdef BINARY_COMPATIBLE_DEBUGLOG
#ifdef DEBUG
#undef eDebug
#define eDebug(...) do { fflush(stdout); printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while (0);
#else
inline void eDebug(const char* fmt, ...) { }
#endif
#endif

eServiceFactoryHisilicon::eServiceFactoryHisilicon()
{
	ePtr<eServiceCenter> sc;

	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		extensions.push_back("dts");
		extensions.push_back("mp3");
		extensions.push_back("wav");
		extensions.push_back("wave");
		extensions.push_back("oga");
		extensions.push_back("ogg");
		extensions.push_back("flac");
		extensions.push_back("m4a");
		extensions.push_back("mp2");
		extensions.push_back("m2a");
		extensions.push_back("wma");
		extensions.push_back("ac3");
		extensions.push_back("mka");
		extensions.push_back("aac");
		extensions.push_back("ape");
		extensions.push_back("alac");
		extensions.push_back("mpg");
		extensions.push_back("vob");
		extensions.push_back("m4v");
		extensions.push_back("mkv");
		extensions.push_back("avi");
		extensions.push_back("divx");
		extensions.push_back("dat");
		extensions.push_back("flv");
		extensions.push_back("mp4");
		extensions.push_back("mov");
		extensions.push_back("wmv");
		extensions.push_back("asf");
		extensions.push_back("3gp");
		extensions.push_back("3g2");
		extensions.push_back("mpeg");
		extensions.push_back("mpe");
		extensions.push_back("rm");
		extensions.push_back("rmvb");
		extensions.push_back("ogm");
		extensions.push_back("ogv");
		extensions.push_back("stream");
		extensions.push_back("webm");
		extensions.push_back("amr");
		extensions.push_back("au");
		extensions.push_back("mid");
		extensions.push_back("wv");
		extensions.push_back("pva");
		extensions.push_back("wtv");
		sc->removeServiceFactory(0x1001); /* servicemp3 */
		sc->addServiceFactory(eServiceFactoryHisilicon::id, this, extensions);
	}

	m_service_info = new eStaticServiceHisiliconInfo();
}

eServiceFactoryHisilicon::~eServiceFactoryHisilicon()
{
	ePtr<eServiceCenter> sc;

	eServiceCenter::getPrivInstance(sc);
	if (sc)
		sc->removeServiceFactory(eServiceFactoryHisilicon::id);
}

DEFINE_REF(eServiceFactoryHisilicon)

	// iServiceHandler
RESULT eServiceFactoryHisilicon::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
		// check resources...
	ptr = new eServiceHisilicon(ref);
	return 0;
}

RESULT eServiceFactoryHisilicon::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryHisilicon::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryHisilicon::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = m_service_info;
	return 0;
}

class eHisiliconServiceOfflineOperations: public iServiceOfflineOperations
{
	DECLARE_REF(eHisiliconServiceOfflineOperations);
	eServiceReference m_ref;
public:
	eHisiliconServiceOfflineOperations(const eServiceReference &ref);

	RESULT deleteFromDisk(int simulate);
	RESULT getListOfFilenames(std::list<std::string> &);
	RESULT reindex();
};

DEFINE_REF(eHisiliconServiceOfflineOperations);

eHisiliconServiceOfflineOperations::eHisiliconServiceOfflineOperations(const eServiceReference &ref): m_ref((const eServiceReference&)ref)
{
}

RESULT eHisiliconServiceOfflineOperations::deleteFromDisk(int simulate)
{
	if (!simulate)
	{
		std::list<std::string> res;
		if (getListOfFilenames(res))
			return -1;

		eBackgroundFileEraser *eraser = eBackgroundFileEraser::getInstance();
		if (!eraser)
			eDebug("[eHisiliconServiceOfflineOperations] FATAL !! can't get background file eraser");

		for (std::list<std::string>::iterator i(res.begin()); i != res.end(); ++i)
		{
			eDebug("[eHisiliconServiceOfflineOperations] Removing %s...", i->c_str());
			if (eraser)
				eraser->erase(i->c_str());
			else
				::unlink(i->c_str());
		}
	}
	return 0;
}

RESULT eHisiliconServiceOfflineOperations::getListOfFilenames(std::list<std::string> &res)
{
	res.clear();
	res.push_back(m_ref.path);
	return 0;
}

RESULT eHisiliconServiceOfflineOperations::reindex()
{
	return -1;
}


RESULT eServiceFactoryHisilicon::offlineOperations(const eServiceReference &ref, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = new eHisiliconServiceOfflineOperations(ref);
	return 0;
}

// eStaticServiceHisiliconInfo


// eStaticServiceHisiliconInfo is seperated from eServiceHisilicon to give information
// about unopened files.

// probably eServiceHisilicon should use this class as well, and eStaticServiceHisiliconInfo
// should have a database backend where ID3-files etc. are cached.
// this would allow listing the Hisilicon database based on certain filters.

DEFINE_REF(eStaticServiceHisiliconInfo)

eStaticServiceHisiliconInfo::eStaticServiceHisiliconInfo()
{
}

RESULT eStaticServiceHisiliconInfo::getName(const eServiceReference &ref, std::string &name)
{
	if ( ref.name.length() )
		name = ref.name;
	else
	{
		size_t last = ref.path.rfind('/');
		if (last != std::string::npos)
			name = ref.path.substr(last+1);
		else
			name = ref.path;
	}
	return 0;
}

int eStaticServiceHisiliconInfo::getLength(const eServiceReference &ref)
{
	return -1;
}

int eStaticServiceHisiliconInfo::getInfo(const eServiceReference &ref, int w)
{
	switch (w)
	{
	case iServiceInformation::sTimeCreate:
		{
			struct stat s;
			if (stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_mtime;
			}
		}
		break;
	case iServiceInformation::sFileSize:
		{
			struct stat s;
			if (stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_size;
			}
		}
		break;
	}
	return iServiceInformation::resNA;
}

long long eStaticServiceHisiliconInfo::getFileSize(const eServiceReference &ref)
{
	struct stat s;
	if (stat(ref.path.c_str(), &s) == 0)
	{
		return s.st_size;
	}
	return 0;
}

RESULT eStaticServiceHisiliconInfo::getEvent(const eServiceReference &ref, ePtr<eServiceEvent> &evt, time_t start_time)
{
	if (ref.path.find("://") != std::string::npos)
	{
		eServiceReference equivalentref(ref);
		equivalentref.type = eServiceFactoryHisilicon::id;
		equivalentref.path.clear();
		return eEPGCache::getInstance()->lookupEventTime(equivalentref, start_time, evt);
	}
	evt = 0;
	return -1;
}

DEFINE_REF(eHisiliconBufferInfo)

eHisiliconBufferInfo::eHisiliconBufferInfo(int percentage, int inputrate, int outputrate, int space, int size)
: bufferPercentage(percentage),
	inputRate(inputrate),
	outputRate(outputrate),
	bufferSpace(space),
	bufferSize(size)
{
}

int eHisiliconBufferInfo::getBufferPercentage() const
{
	return bufferPercentage;
}

int eHisiliconBufferInfo::getAverageInputRate() const
{
	return inputRate;
}

int eHisiliconBufferInfo::getAverageOutputRate() const
{
	return outputRate;
}

int eHisiliconBufferInfo::getBufferSpace() const
{
	return bufferSpace;
}

int eHisiliconBufferInfo::getBufferSize() const
{
	return bufferSize;
}

DEFINE_REF(eServiceHisiliconInfoContainer);

eServiceHisiliconInfoContainer::eServiceHisiliconInfoContainer()
: doubleValue(0.0), bufferData(NULL), bufferSize(0)
{
}

eServiceHisiliconInfoContainer::~eServiceHisiliconInfoContainer()
{
}

double eServiceHisiliconInfoContainer::getDouble(unsigned int index) const
{
	return doubleValue;
}

unsigned char *eServiceHisiliconInfoContainer::getBuffer(unsigned int &size) const
{
	size = bufferSize;
	return bufferData;
}

void eServiceHisiliconInfoContainer::setDouble(double value)
{
	doubleValue = value;
}

// eServiceHisilicon
int eServiceHisilicon::ac3_delay = 0,
	eServiceHisilicon::pcm_delay = 0;

#define MAX_PAYLOAD 4096

void eServiceHisilicon::video_event(int)
{
	while (m_video_fd >= 0)
	{
		int retval;
		pollfd pfd[1];
		pfd[0].fd = m_video_fd;
		pfd[0].events = POLLPRI;
		retval = ::poll(pfd, 1, 0);
		if (retval < 0 && errno == EINTR) continue;
		if (retval <= 0) break;
		struct video_event evt;
		if (::ioctl(m_video_fd, VIDEO_GET_EVENT, &evt) < 0)
		{
			eDebug("[eServiceHisilicon] VIDEO_GET_EVENT failed: %m");
			break;
		}
		else
		{
			if (evt.type == VIDEO_EVENT_SIZE_CHANGED)
			{
				m_aspect = evt.u.size.aspect_ratio == 0 ? 2 : 3;  // convert dvb api to etsi
				m_height = evt.u.size.h;
				m_width = evt.u.size.w;
				eDebug("[eServiceHisilicon] SIZE_CHANGED %dx%d aspect %d", m_width, m_height, m_aspect);
				m_event((iPlayableService*)this, evVideoSizeChanged);
			}
			else if (evt.type == VIDEO_EVENT_FRAME_RATE_CHANGED)
			{
				m_framerate = evt.u.frame_rate;
				eDebug("[eServiceHisilicon] FRAME_RATE_CHANGED %d fps", m_framerate);
				m_event((iPlayableService*)this, evVideoFramerateChanged);
			}
			else if (evt.type == 16 /*VIDEO_EVENT_PROGRESSIVE_CHANGED*/)
			{
				m_progressive = evt.u.frame_rate;
				eDebug("[eServiceHisilicon] PROGRESSIVE_CHANGED %d", m_progressive);
				m_event((iPlayableService*)this, evVideoProgressiveChanged);
			}
			else if (evt.type == 17 /*VIDEO_EVENT_GAMMA_CHANGED*/)
			{
				m_gamma = evt.u.frame_rate;
				eDebug("[eServiceHisilicon] GAMMA_CHANGED %d", m_gamma);
				m_event((iPlayableService*)this, evVideoGammaChanged);
			}
			else
				eDebug("[eServiceHisilicon] unhandled DVBAPI Video Event %d", evt.type);
		}
	}
}

int eServiceHisilicon::receive_netlink_message(void)
{
	struct msghdr msg;
	struct iovec iov;
	struct sockaddr_nl dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */
	dest_addr.nl_groups = 0; /* unicast */

	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	if (recvmsg(netlink_socket, &msg, 0) <= 0)
	{
		return 0;
	}
	return NLMSG_PAYLOAD(nlh, 0);
}

void eServiceHisilicon::netlink_event(int)
{
	if (netlink_socket >= 0)
	{
		int readlen = receive_netlink_message();
		if (readlen > 0)
		{
			int decoder = *((uint32_t*)NLMSG_DATA(nlh)) >> 24;
			int msgtype = *((uint32_t*)NLMSG_DATA(nlh)) & 0xffffff;
			switch (msgtype)
			{
			case 2: /* subtitle data */
				if (m_currentSubtitleStream >= 0 && m_subtitle_widget && !m_paused)
				{
					struct subtitleheader
					{
						uint64_t pts;
						uint32_t duration;
						uint32_t datasize;
					} header;
					memcpy(&header, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(header));
					ePangoSubtitlePage pango_page;
					gRGB rgbcol(0xD0,0xD0,0xD0);
					pango_page.m_elements.push_back(ePangoSubtitlePageElement(rgbcol, (const char*)NLMSG_DATA(nlh) + sizeof(uint32_t) + sizeof(header)));
					pango_page.m_show_pts = header.pts; /* ignored by widget (TODO?) */
					pango_page.m_timeout = 8000;
					m_subtitle_widget->setPage(pango_page);
				}
				break;
			case 3: /* clear subtitles */
				if (m_currentSubtitleStream >= 0 && m_subtitle_widget)
				{
					ePangoSubtitlePage pango_page;
					pango_page.m_show_pts = 0;
					pango_page.m_timeout = 0;
					m_subtitle_widget->setPage(pango_page);
				}
				break;
			case 4: /* file info */
				memcpy(&fileinfo, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(fileinfo));
				fileinfo.pastProgramInfo = (HI_FORMAT_PROGRAM_INFO_S*)malloc(fileinfo.u32ProgramNum * sizeof(HI_FORMAT_PROGRAM_INFO_S));
				fileinfo.u32ProgramNum = 0;
				break;
			case 5: /* program info */
				memcpy(&fileinfo.pastProgramInfo[fileinfo.u32ProgramNum], (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(HI_FORMAT_PROGRAM_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].pastVidStream = (HI_FORMAT_VID_INFO_S*)malloc(fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].u32VidStreamNum * sizeof(HI_FORMAT_VID_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].u32VidStreamNum = 0;
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].pastAudStream = (HI_FORMAT_AUD_INFO_S*)malloc(fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].u32AudStreamNum * sizeof(HI_FORMAT_AUD_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].u32AudStreamNum = 0;
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].pastSubStream = (HI_FORMAT_SUB_INFO_S*)malloc(fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].u32SubStreamNum * sizeof(HI_FORMAT_SUB_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum].u32SubStreamNum = 0;
				fileinfo.u32ProgramNum++;
				break;
			case 6: /* video stream */
				memcpy(&fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].pastVidStream[fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].u32VidStreamNum], (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(HI_FORMAT_VID_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].u32VidStreamNum++;
				break;
			case 7: /* audio stream */
				memcpy(&fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].pastAudStream[fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].u32AudStreamNum], (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(HI_FORMAT_AUD_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].u32AudStreamNum++;
				break;
			case 8: /* subtitle stream */
				memcpy(&fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].pastSubStream[fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].u32SubStreamNum], (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(HI_FORMAT_SUB_INFO_S));
				fileinfo.pastProgramInfo[fileinfo.u32ProgramNum - 1].u32SubStreamNum++;
				break;
			case 9: /* stream id */
				memcpy(&streamid, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(streamid));
				eDebug("[eServiceHisilicon] streamid: program %u, video %u, audio %u, subtitle %u", streamid.programid, streamid.videostreamid, streamid.audiostreamid, streamid.subtitlestreamid);
				m_event((iPlayableService*)this, evUpdatedInfo);
				break;
			case 10: /* player state */
				{
					int32_t state;
					memcpy(&state, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(state));
					eDebug("[eServiceHisilicon] player state %d-->%d", m_player_state, state);
					switch (state)
					{
					case 0: /* init */
						break;
					case 1: /* deinit */
						break;
					case 2: /* play */
						if (m_state != stRunning)
						{
							m_state = stRunning;
							m_event((iPlayableService*)this, evStart);
						}
						m_paused = false;
						break;
					case 3: /* fast forward */
						break;
					case 4: /* rewind */
						break;
					case 5: /* pause */
						m_paused = true;
						break;
					case 6: /* stop */
						m_paused = false;
						m_event((iPlayableService*)this, evStopped);
						break;
					case 7: /* preparing */
						break;
					}
					m_player_state = state;
				}
				break;
			case 11: /* error */
				{
					int32_t error;
					memcpy(&error, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(error));
					eDebug("[eServiceHisilicon] error %d", error);
					switch (error)
					{
					case 0: /* no error */
						break;
					case 1: /* video playback failed */
						m_errorInfo.error_message = "video playback failed";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 2: /* audio playback failed */
						m_errorInfo.error_message = "audio playback failed";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 3: /* subtitle playback failed */
						m_errorInfo.error_message = "subtitle playback failed";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 4: /* media playback failed */
						m_errorInfo.error_message = "media playback failed";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 5: /* timeout */
						m_errorInfo.error_message = "timeout";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 6: /* file format not supported */
						m_errorInfo.error_message = "format not supported";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 7: /* unknown error */
						m_errorInfo.error_message = "unknown error";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 8: /* I-frame decoding error */
						break;
					}
				}
				break;
			case 12: /* buffering */
				{
					struct bufferinfo
					{
						uint32_t status;
						uint32_t percentage;
					} info;
					memcpy(&info, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(info));
					eDebug("[eServiceHisilicon] buffering %u %u%%", info.status, info.percentage);
					m_bufferpercentage = info.percentage;
					switch (info.status)
					{
					case 0: /* empty */
					case 1: /* insufficient */
						if (!m_buffering)
						{
							m_buffering = true;
							m_event((iPlayableService*)this, evBuffering);
							pause();
						}
						break;
					case 2: /* enough */
					case 3: /* full */
						if (m_buffering)
						{
							m_buffering = false;
							m_event((iPlayableService*)this, evGstreamerPlayStarted);
							unpause();
						}
						break;
					default:
						break;
					}
				}
				break;
			case 13: /* network info */
				{
					struct networkinfo
					{
						uint32_t status;
						int32_t errorcode;
					} info;
					memcpy(&info, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(info));
					eDebug("[eServiceHisilicon] network info %u %d", info.status, info.errorcode);
					switch (info.status)
					{
					case 0: /* network: unknown error */
						m_errorInfo.error_message = "network: error";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 1: /* network: failed to connect */
						m_errorInfo.error_message = "network: connection failed";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 2: /* network: timeout */
						m_errorInfo.error_message = "network: timeout";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 3: /* network: disconnected */
						m_errorInfo.error_message = "network: disconnected";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 4: /* network: file not found */
						m_errorInfo.error_message = "network: file not found";
						m_event((iPlayableService*)this, evUser+12);
						break;
					case 5: /* network: status ok */
						break;
					case 6: /* network: http errorcode */
						break;
					case 7: /* network: bitrate adjusted */
						break;
					}
				}
				break;
			case 14: /* event */
				{
					int32_t event;
					memcpy(&event, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(event));
					switch (event)
					{
					case 0: /* SOF */
						eDebug("[eServiceHisilicon] event: SOF");
						/* reached SOF while rewinding */
						m_event((iPlayableService*)this, evSOF);
						break;
					case 1: /* EOF */
						eDebug("[eServiceHisilicon] event: EOF");
						m_event((iPlayableService*)this, evEOF);
						break;
					}
				}
				break;
			case 15: /* seekable */
				memcpy(&m_seekable, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(m_seekable));
				break;
			case 16: /* download progress */
				memcpy(&m_download_progress, (unsigned char*)NLMSG_DATA(nlh) + sizeof(uint32_t), sizeof(m_download_progress));
				if (m_download_progress >= 100)
				{
					m_event((iPlayableService*)this, evGstreamerPlayStarted);
					unpause();
				}
				break;
			default:
				break;
			}
		}
	}
}

eServiceHisilicon::eServiceHisilicon(eServiceReference ref):
	m_nownext_timer(eTimer::create(eApp)),
	m_cuesheet_changed(0),
	m_cutlist_enabled(1),
	m_ref(ref)
{
	m_currentAudioStream = -1;
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = -2; /* report subtitle stream to be 'cached'. TODO: use an actual cache. */
	m_subtitle_widget = 0;
	m_buffering = false;
	m_paused = false;
	m_player_state = 0;
	m_cuesheet_loaded = false; /* cuesheet CVR */
	m_seekable = 0;
	m_bufferpercentage = 0;

	m_useragent = "Enigma2 HbbTV/1.1.1 (+PVR+RTSP+DL;OpenPLi;;;)";
	m_extra_headers = "";
	m_download_buffer_path = "";
	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	m_errorInfo.missing_codec = "";

	CONNECT(m_nownext_timer->timeout, eServiceHisilicon::updateEpgCacheNowNext);
	m_aspect = m_width = m_height = m_framerate = m_progressive = m_gamma = -1;

	m_state = stIdle;
	eDebug("[eServiceHisilicon] construct!");

	if (eConfigManager::getConfigBoolValue("config.mediaplayer.useAlternateUserAgent"))
		m_useragent = eConfigManager::getConfigValue("config.mediaplayer.alternateUserAgent");

	const char *filename;
	std::string filename_str;
	size_t pos = m_ref.path.find('#');
	if (pos != std::string::npos && (m_ref.path.compare(0, 4, "http") == 0 || m_ref.path.compare(0, 4, "rtsp") == 0))
	{
		filename_str = m_ref.path.substr(0, pos);
		filename = filename_str.c_str();
		m_extra_headers = m_ref.path.substr(pos + 1);

		pos = m_extra_headers.find("User-Agent=");
		if (pos != std::string::npos)
		{
			size_t hpos_start = pos + 11;
			size_t hpos_end = m_extra_headers.find('&', hpos_start);
			if (hpos_end != std::string::npos)
				m_useragent = m_extra_headers.substr(hpos_start, hpos_end - hpos_start);
			else
				m_useragent = m_extra_headers.substr(hpos_start);
		}
	}
	else
		filename = m_ref.path.c_str();
	const char *ext = strrchr(filename, '.');
	if (!ext)
		ext = filename + strlen(filename);

	pos = m_ref.path.find("&suburi=");
	if (pos != std::string::npos)
	{
		filename_str = filename;

		std::string suburi_str = filename_str.substr(pos + 8);
		filename = suburi_str.c_str();

		filename_str = filename_str.substr(0, pos);
		filename = filename_str.c_str();
	}

	memset(&fileinfo, 0, sizeof(fileinfo));
	memset(&streamid, 0, sizeof(streamid));

	eDebug("[eServiceHisilicon] uri=%s", filename);

	{
		struct sockaddr_nl src_addr;
		memset(&src_addr, 0, sizeof(src_addr));
		src_addr.nl_family = AF_NETLINK;
		src_addr.nl_pid = getpid(); /* self pid */
		netlink_socket = socket(PF_NETLINK, SOCK_RAW, 30);
		bind(netlink_socket, (struct sockaddr*)&src_addr, sizeof(src_addr));
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));

	m_sn_netlink = eSocketNotifier::create(eApp, netlink_socket, eSocketNotifier::Read);
	CONNECT(m_sn_netlink->activated, eServiceHisilicon::netlink_event);

	m_audio_fd = ::open("/dev/dvb/adapter0/audio0", O_RDWR | O_CLOEXEC);
	m_video_fd = ::open("/dev/dvb/adapter0/video0", O_RDWR | O_CLOEXEC);
	m_sn = eSocketNotifier::create(eApp, m_video_fd, eSocketNotifier::Priority);
	CONNECT(m_sn->activated, eServiceHisilicon::video_event);

	if (m_video_fd >= 0)
	{
		struct video_command cmd = {0};
		struct argdata
		{
			uint32_t size;
			void *arg;
		} *argdata = (struct argdata*)cmd.raw.data;
		cmd.cmd = 101; /* set url */
		argdata->arg = (void*)filename;
		argdata->size = strlen(filename) + 1;
		::ioctl(m_video_fd, VIDEO_COMMAND, &cmd);
		cmd.cmd = 102; /* set useragent */
		argdata->arg = (void*)m_useragent.c_str();
		argdata->size = m_useragent.length() + 1;
		::ioctl(m_video_fd, VIDEO_COMMAND, &cmd);
	}
}

eServiceHisilicon::~eServiceHisilicon()
{
	m_currentSubtitleStream = -1;
	if (m_subtitle_widget) m_subtitle_widget->destroy();
	m_subtitle_widget = 0;

	stop();

	if (netlink_socket >= 0)
	{
		::close(netlink_socket);
	}
	free(nlh);
	if (m_audio_fd >= 0)
	{
		::close(m_audio_fd);
	}
	if (m_video_fd >= 0)
	{
		::close(m_video_fd);
	}

	unsigned int i;
	for (i = 0; i < fileinfo.u32ProgramNum; i++)
	{
		free(fileinfo.pastProgramInfo[i].pastVidStream);
		free(fileinfo.pastProgramInfo[i].pastAudStream);
		free(fileinfo.pastProgramInfo[i].pastSubStream);
	}
	free(fileinfo.pastProgramInfo);
}

void eServiceHisilicon::updateEpgCacheNowNext()
{
	bool update = false;
	ePtr<eServiceEvent> next = 0;
	ePtr<eServiceEvent> ptr = 0;
	eServiceReference ref(m_ref);
	ref.type = eServiceFactoryHisilicon::id;
	ref.path.clear();
	if (eEPGCache::getInstance() && eEPGCache::getInstance()->lookupEventTime(ref, -1, ptr) >= 0)
	{
		ePtr<eServiceEvent> current = m_event_now;
		if (!current || !ptr || current->getEventId() != ptr->getEventId())
		{
			update = true;
			m_event_now = ptr;
			time_t next_time = ptr->getBeginTime() + ptr->getDuration();
			if (eEPGCache::getInstance()->lookupEventTime(ref, next_time, ptr) >= 0)
			{
				next = ptr;
				m_event_next = ptr;
			}
		}
	}

	int refreshtime = 60;
	if (!next)
	{
		next = m_event_next;
	}
	if (next)
	{
		time_t now = eDVBLocalTimeHandler::getInstance()->nowTime();
		refreshtime = (int)(next->getBeginTime() - now) + 3;
		if (refreshtime <= 0 || refreshtime > 60)
		{
			refreshtime = 60;
		}
	}
	m_nownext_timer->startLongTimer(refreshtime);
	if (update)
	{
		m_event((iPlayableService*)this, evUpdatedEventInfo);
	}
}

DEFINE_REF(eServiceHisilicon);

RESULT eServiceHisilicon::connectEvent(const sigc::slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
	return 0;
}

RESULT eServiceHisilicon::start()
{
	ASSERT(m_state == stIdle);

	if (m_video_fd >= 0)
	{
		::ioctl(m_video_fd, VIDEO_FAST_FORWARD, 0);
		::ioctl(m_video_fd, VIDEO_SLOWMOTION, 0);
		::ioctl(m_video_fd, VIDEO_PLAY);
		::ioctl(m_video_fd, VIDEO_CONTINUE);
	}
	if (m_audio_fd >= 0)
	{
		::ioctl(m_audio_fd, AUDIO_PLAY);
		::ioctl(m_audio_fd, AUDIO_CONTINUE);
	}
	return 0;
}

RESULT eServiceHisilicon::stop()
{
	if (m_state == stStopped)
		return -1;

	eDebug("[eServiceHisilicon] stop %s", m_ref.path.c_str());

	if (m_video_fd >= 0)
	{
		::ioctl(m_video_fd, VIDEO_STOP);
	}
	if (m_audio_fd >= 0)
	{
		::ioctl(m_audio_fd, AUDIO_STOP);
	}

	m_state = stStopped;

	saveCuesheet();
	m_nownext_timer->stop();

	return 0;
}

RESULT eServiceHisilicon::pause(ePtr<iPauseableService> &ptr)
{
	ptr=this;
	return 0;
}

RESULT eServiceHisilicon::setSlowMotion(int ratio)
{
	if (!ratio)
		return 0;
	eDebug("[eServiceHisilicon] setSlowMotion ratio=%i", ratio);
	if (m_video_fd >= 0)
	{
		::ioctl(m_video_fd, VIDEO_SLOWMOTION, ratio);
		::ioctl(m_video_fd, VIDEO_CONTINUE);
	}
	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	return 0;
}

RESULT eServiceHisilicon::setFastForward(int ratio)
{
	eDebug("[eServiceHisilicon] setFastForward ratio=%i",ratio);
	if (m_video_fd >= 0)
	{
		::ioctl(m_video_fd, VIDEO_FAST_FORWARD, ratio);
		::ioctl(m_video_fd, VIDEO_CONTINUE);
	}
	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	return 0;
}

		// iPausableService
RESULT eServiceHisilicon::pause()
{
	if (m_state != stRunning)
		return -1;

	eDebug("[eServiceHisilicon] pause");
	if (m_video_fd >= 0)
	{
		::ioctl(m_video_fd, VIDEO_FREEZE);
	}
	if (m_audio_fd >= 0)
	{
		::ioctl(m_audio_fd, AUDIO_PAUSE);
	}

	return 0;
}

RESULT eServiceHisilicon::unpause()
{
	if (m_state != stRunning)
		return -1;

	eDebug("[eServiceHisilicon] unpause");

	if (m_video_fd >= 0)
	{
		::ioctl(m_video_fd, VIDEO_FAST_FORWARD, 0);
		::ioctl(m_video_fd, VIDEO_SLOWMOTION, 0);
		::ioctl(m_video_fd, VIDEO_CONTINUE);
	}
	if (m_audio_fd >= 0)
	{
		::ioctl(m_audio_fd, AUDIO_CONTINUE);
	}

	return 0;
}

	/* iSeekableService */
RESULT eServiceHisilicon::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceHisilicon::getLength(pts_t &pts)
{
	if (m_state != stRunning)
		return -1;
	pts = fileinfo.s64Duration * 90LL;
	return 0;
}

RESULT eServiceHisilicon::seekToImpl(pts_t to)
{
	if (m_video_fd >= 0)
	{
		struct video_command cmd = {0};
		cmd.cmd = 100; /* seek */
		cmd.stop.pts = to;
		::ioctl(m_video_fd, VIDEO_COMMAND, &cmd);
	}

	if (m_paused)
	{
		m_event((iPlayableService*)this, evUpdatedInfo);
	}

	return 0;
}

RESULT eServiceHisilicon::seekTo(pts_t to)
{
	RESULT ret = -1;

	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	ret = seekToImpl(to);

	return ret;
}

RESULT eServiceHisilicon::seekRelative(int direction, pts_t to)
{
	pts_t ppos;
	if (getPlayPosition(ppos) < 0) return -1;
	ppos += to * direction;
	if (ppos < 0)
		ppos = 0;
	return seekTo(ppos);
}

RESULT eServiceHisilicon::getPlayPosition(pts_t &pts)
{
	pts = 0;

	if (m_video_fd >= 0)
	{
		if (::ioctl(m_video_fd, VIDEO_GET_PTS, &pts) >= 0)
		{
			return 0;
		}
	}
	if (m_audio_fd >= 0)
	{
		if (::ioctl(m_audio_fd, AUDIO_GET_PTS, &pts) >= 0)
		{
			return 0;
		}
	}
	return 0;
}

RESULT eServiceHisilicon::setTrickmode(int trick)
{
	/* TODO */
	return -1;
}

RESULT eServiceHisilicon::isCurrentlySeekable()
{
	return m_seekable ? 3 : 0;
}

RESULT eServiceHisilicon::info(ePtr<iServiceInformation>&i)
{
	i = this;
	return 0;
}

RESULT eServiceHisilicon::getName(std::string &name)
{
	std::string title = m_ref.getName();
	if (title.empty())
	{
		name = m_ref.path;
		size_t n = name.rfind('/');
		if (n != std::string::npos)
			name = name.substr(n + 1);
	}
	else
		name = title;
	return 0;
}

RESULT eServiceHisilicon::getEvent(ePtr<eServiceEvent> &evt, int nownext)
{
	evt = nownext ? m_event_next : m_event_now;
	if (!evt)
		return -1;
	return 0;
}

int eServiceHisilicon::getInfo(int w)
{
	HI_FORMAT_VID_INFO_S *pstVidStream = NULL;
	if (fileinfo.u32ProgramNum > 0 && streamid.programid < fileinfo.u32ProgramNum)
	{
		if (fileinfo.pastProgramInfo[streamid.programid].u32VidStreamNum > 0)
		{
			pstVidStream = &fileinfo.pastProgramInfo[streamid.programid].pastVidStream[streamid.videostreamid];
		}
	}

	switch (w)
	{
	case sServiceref: return m_ref;
	case sVideoHeight: return m_height;
	case sVideoWidth: return m_width;
	case sFrameRate: return m_framerate;
	case sProgressive: return m_progressive;
	case sGamma: return m_gamma;
	case sAspect: return m_aspect;
	case sTagTitle:
	case sTagArtist:
	case sTagAlbum:
	case sTagTitleSortname:
	case sTagArtistSortname:
	case sTagAlbumSortname:
	case sTagDate:
	case sTagComposer:
	case sTagGenre:
	case sTagComment:
	case sTagExtendedComment:
	case sTagLocation:
	case sTagHomepage:
	case sTagDescription:
	case sTagVersion:
	case sTagISRC:
	case sTagOrganization:
	case sTagCopyright:
	case sTagCopyrightURI:
	case sTagContact:
	case sTagLicense:
	case sTagLicenseURI:
	case sTagCodec:
	case sTagAudioCodec:
	case sTagVideoCodec:
	case sTagEncoder:
	case sTagLanguageCode:
	case sTagKeywords:
	case sTagChannelMode:
	case sUser+12:
		return resIsString;
	case sTagTrackGain:
	case sTagTrackPeak:
	case sTagAlbumGain:
	case sTagAlbumPeak:
	case sTagReferenceLevel:
	case sTagBeatsPerMinute:
	case sTagImage:
	case sTagPreviewImage:
	case sTagAttachment:
		return resIsPyObject;
	case sTagTrackNumber:
		return streamid.programid;
	case sTagTrackCount:
		return fileinfo.u32ProgramNum;
	case sTagAlbumVolumeNumber:
		return resNA;
	case sTagAlbumVolumeCount:
		return resNA;
	case sTagBitrate:
		return fileinfo.u32Bitrate;
	case sTagNominalBitrate:
		return fileinfo.u32Bitrate;
	case sTagMinimumBitrate:
		return fileinfo.u32Bitrate;
	case sTagMaximumBitrate:
		return fileinfo.u32Bitrate;
	case sTagSerial:
		return resNA;
	case sTagEncoderVersion:
		if (pstVidStream)
		{
			return pstVidStream->u32CodecVersion;
		}
		break;
	case sTagCRC:
		return resNA;
#if 0
	case sBuffer:
	{
		HI_FORMAT_BUFFER_STATUS_S status;
		HI_FORMAT_BUFFER_CONFIG_S config;
		HI_SVR_PLAYER_Invoke(m_player, HI_FORMAT_INVOKE_GET_BUFFER_CONFIG, &config);
		HI_SVR_PLAYER_Invoke(m_player, HI_FORMAT_INVOKE_GET_BUFFER_STATUS, &status);
		return status.s64BufferSize * 100LL / config.s64Total;
	}
#endif
	default:
		return resNA;
	}
	return 0;
}

const char *eServiceHisilicon::getVidFormatStr(uint32_t format)
{
	switch (format)
	{
	case HI_FORMAT_VIDEO_MPEG2:
		return "MPEG2";
		break;
	case HI_FORMAT_VIDEO_MPEG4:
		return "MPEG4";
		break;
	case HI_FORMAT_VIDEO_AVS:
		return "AVS";
		break;
	case HI_FORMAT_VIDEO_H263:
		return "H263";
		break;
	case HI_FORMAT_VIDEO_H264:
		return "H264";
		break;
	case HI_FORMAT_VIDEO_REAL8:
		return "REAL8";
		break;
	case HI_FORMAT_VIDEO_REAL9:
		return "REAL9";
		break;
	case HI_FORMAT_VIDEO_VC1:
		return "VC1";
		break;
	case HI_FORMAT_VIDEO_VP6:
		return "VP6";
		break;
	case HI_FORMAT_VIDEO_VP6F:
		return "VP6F";
		break;
	case HI_FORMAT_VIDEO_VP6A:
		return "VP6A";
		break;
	case HI_FORMAT_VIDEO_MJPEG:
		return "MJPEG";
		break;
	case HI_FORMAT_VIDEO_SORENSON:
		return "SORENSON";
		break;
	case HI_FORMAT_VIDEO_DIVX3:
		return "DIVX3";
		break;
	case HI_FORMAT_VIDEO_RAW:
		return "RAW";
		break;
	case HI_FORMAT_VIDEO_JPEG:
		return "JPEG";
		break;
	case HI_FORMAT_VIDEO_VP8:
		return "VP8";
		break;
	case HI_FORMAT_VIDEO_MSMPEG4V1:
		return "MICROSOFT MPEG4V1";
		break;
	case HI_FORMAT_VIDEO_MSMPEG4V2:
		return "MICROSOFT MPEG4V2";
		break;
	case HI_FORMAT_VIDEO_WMV1:
		return "WMV1";
		break;
	case HI_FORMAT_VIDEO_WMV2:
		return "WMV2";
		break;
	case HI_FORMAT_VIDEO_RV10:
		return "RV10";
		break;
	case HI_FORMAT_VIDEO_RV20:
		return "RV20";
		break;
	case HI_FORMAT_VIDEO_SVQ1:
		return "SORENSON1";
		break;
	case HI_FORMAT_VIDEO_SVQ3:
		return "SORENSON3";
		break;
	case HI_FORMAT_VIDEO_VP3:
		return "VP3";
		break;
	case HI_FORMAT_VIDEO_VP5:
		return "VP5";
		break;
	case HI_FORMAT_VIDEO_CINEPAK:
		return "CINEPACK";
		break;
	case HI_FORMAT_VIDEO_INDEO2:
		return "INDEO2";
		break;
	case HI_FORMAT_VIDEO_INDEO3:
		return "INDEO3";
		break;
	case HI_FORMAT_VIDEO_INDEO4:
		return "INDEO4";
		break;
	case HI_FORMAT_VIDEO_INDEO5:
		return "INDEO5";
		break;
	case HI_FORMAT_VIDEO_MJPEGB:
		return "MJPEGB";
		break;
	case HI_FORMAT_VIDEO_MVC:
		return "MVC";
		break;
	case HI_FORMAT_VIDEO_HEVC:
		return "h265";
		break;
	case HI_FORMAT_VIDEO_DV:
		return "DV";
		break;
	case HI_FORMAT_VIDEO_HUFFYUV:
		return "HUFFYUV";
		break;
	case HI_FORMAT_VIDEO_DIVX:
		return "DIVX";
		break;
	case HI_FORMAT_VIDEO_REALMAGICMPEG4:
		return "REALMAGICMPEG4";
		break;
	case HI_FORMAT_VIDEO_VP9:
		return "VP9";
		break;
	case HI_FORMAT_VIDEO_WMV3:
		return "WMV3";
		break;
	case HI_FORMAT_VIDEO_AVS2:
		return "AVS2";
		break;
	case HI_FORMAT_VIDEO_BUTT:
		return "BUTT";
		break;

	default:
		return "UNKNOWN";
		break;
	}

	return "UNKNOWN";
}

const char *eServiceHisilicon::getAudFormatStr(uint32_t format)
{
	switch (format)
	{
	case HI_FORMAT_AUDIO_MP2:
		return "MPEG2";
		break;
	case HI_FORMAT_AUDIO_MP3:
		return "MPEG3";
		break;
	case HI_FORMAT_AUDIO_AAC:
		return "AAC";
		break;
	case HI_FORMAT_AUDIO_AC3:
		return "AC3";
		break;
	case HI_FORMAT_AUDIO_DTS:
		return "DTS";
		break;
	case HI_FORMAT_AUDIO_VORBIS:
		return "VORBIS";
		break;
	case HI_FORMAT_AUDIO_DVAUDIO:
		return "DVAUDIO";
		break;
	case HI_FORMAT_AUDIO_WMAV1:
		return "WMAV1";
		break;
	case HI_FORMAT_AUDIO_WMAV2:
		return "WMAV2";
		break;
	case HI_FORMAT_AUDIO_MACE3:
		return "MACE3";
		break;
	case HI_FORMAT_AUDIO_MACE6:
		return "MACE6";
		break;
	case HI_FORMAT_AUDIO_VMDAUDIO:
		return "VMDAUDIO";
		break;
	case HI_FORMAT_AUDIO_SONIC:
		return "SONIC";
		break;
	case HI_FORMAT_AUDIO_SONIC_LS:
		return "SONIC_LS";
		break;
	case HI_FORMAT_AUDIO_FLAC:
		return "FLAC";
		break;
	case HI_FORMAT_AUDIO_MP3ADU:
		return "MP3ADU";
		break;
	case HI_FORMAT_AUDIO_MP3ON4:
		return "MP3ON4";
		break;
	case HI_FORMAT_AUDIO_SHORTEN:
		return "SHORTEN";
		break;
	case HI_FORMAT_AUDIO_ALAC:
		return "ALAC";
		break;
	case HI_FORMAT_AUDIO_WESTWOOD_SND1:
		return "WESTWOOD_SND1";
		break;
	case HI_FORMAT_AUDIO_GSM:
		return "GSM";
		break;
	case HI_FORMAT_AUDIO_QDM2:
		return "QDM2";
		break;
	case HI_FORMAT_AUDIO_COOK:
		return "COOK";
		break;
	case HI_FORMAT_AUDIO_TRUESPEECH:
		return "TRUESPEECH";
		break;
	case HI_FORMAT_AUDIO_TTA:
		return "TTA";
		break;
	case HI_FORMAT_AUDIO_SMACKAUDIO:
		return "SMACKAUDIO";
		break;
	case HI_FORMAT_AUDIO_QCELP:
		return "QCELP";
		break;
	case HI_FORMAT_AUDIO_WAVPACK:
		return "WAVPACK";
		break;
	case HI_FORMAT_AUDIO_DSICINAUDIO:
		return "DSICINAUDIO";
		break;
	case HI_FORMAT_AUDIO_IMC:
		return "IMC";
		break;
	case HI_FORMAT_AUDIO_MUSEPACK7:
		return "MUSEPACK7";
		break;
	case HI_FORMAT_AUDIO_MLP:
		return "MLP";
		break;
	case HI_FORMAT_AUDIO_GSM_MS:
		return "GSM_MS";
		break;
	case HI_FORMAT_AUDIO_ATRAC3:
		return "ATRAC3";
		break;
	case HI_FORMAT_AUDIO_VOXWARE:
		return "VOXWARE";
		break;
	case HI_FORMAT_AUDIO_APE:
		return "APE";
		break;
	case HI_FORMAT_AUDIO_NELLYMOSER:
		return "NELLYMOSER";
		break;
	case HI_FORMAT_AUDIO_MUSEPACK8:
		return "MUSEPACK8";
		break;
	case HI_FORMAT_AUDIO_SPEEX:
		return "SPEEX";
		break;
	case HI_FORMAT_AUDIO_WMAVOICE:
		return "WMAVOICE";
		break;
	case HI_FORMAT_AUDIO_WMAPRO:
		return "WMAPRO";
		break;
	case HI_FORMAT_AUDIO_WMALOSSLESS:
		return "WMALOSSLESS";
		break;
	case HI_FORMAT_AUDIO_ATRAC3P:
		return "ATRAC3P";
		break;
	case HI_FORMAT_AUDIO_EAC3:
		return "EAC3";
		break;
	case HI_FORMAT_AUDIO_SIPR:
		return "SIPR";
		break;
	case HI_FORMAT_AUDIO_MP1:
		return "MP1";
		break;
	case HI_FORMAT_AUDIO_TWINVQ:
		return "TWINVQ";
		break;
	case HI_FORMAT_AUDIO_TRUEHD:
		return "TRUEHD";
		break;
	case HI_FORMAT_AUDIO_MP4ALS:
		return "MP4ALS";
		break;
	case HI_FORMAT_AUDIO_ATRAC1:
		return "ATRAC1";
		break;
	case HI_FORMAT_AUDIO_BINKAUDIO_RDFT:
		return "BINKAUDIO_RDFT";
		break;
	case HI_FORMAT_AUDIO_BINKAUDIO_DCT:
		return "BINKAUDIO_DCT";
		break;
	case HI_FORMAT_AUDIO_DRA:
		return "DRA";
		break;
	case HI_FORMAT_AUDIO_AC4:
		return "AC4";
		break;
	case HI_FORMAT_AUDIO_DTS_EXPRESS:
		return "DTS_EXPRESS";
		break;

	case HI_FORMAT_AUDIO_PCM: /* various PCM "codecs" */
		return "PCM";
		break;
	case HI_FORMAT_AUDIO_PCM_BLURAY:
		return "PCM_BLURAY";
		break;

	case HI_FORMAT_AUDIO_ADPCM: /* various ADPCM codecs */
		return "ADPCM";
		break;

	case HI_FORMAT_AUDIO_AMR_NB: /* AMR */
		return "AMR_NB";
		break;
	case HI_FORMAT_AUDIO_AMR_WB:
		return "AMR_WB";
		break;
	case HI_FORMAT_AUDIO_AMR_AWB:
		return "AMR_AWB";
		break;

	case HI_FORMAT_AUDIO_RA_144: /* RealAudio codecs*/
		return "RA_144";
		break;
	case HI_FORMAT_AUDIO_RA_288:
		return "RA_288";
		break;

	case HI_FORMAT_AUDIO_DPCM: /* various DPCM codecs */
		return "DPCM";
		break;

	case HI_FORMAT_AUDIO_G711:  /* various G.7xx codecs */
		return "G711";
		break;
	case HI_FORMAT_AUDIO_G722:
		return "G722";
		break;
	case HI_FORMAT_AUDIO_G7231:
		return "G7231";
		break;
	case HI_FORMAT_AUDIO_G726:
		return "G726";
		break;
	case HI_FORMAT_AUDIO_G728:
		return "G728";
		break;
	case HI_FORMAT_AUDIO_G729AB:
		return "G729AB";
		break;

	case HI_FORMAT_AUDIO_OPUS:
		return "OPUS";
		break;

	default:
		break;
	}

	return "UNKNOWN";
}

const char *eServiceHisilicon::getSubFormatStr(uint32_t format)
{
	switch (format)
	{
	case HI_FORMAT_SUBTITLE_ASS:
		return "ASS";
		break;
	case HI_FORMAT_SUBTITLE_LRC:
		return "LRC";
		break;
	case HI_FORMAT_SUBTITLE_SRT:
		return "SRT";
		break;
	case HI_FORMAT_SUBTITLE_SMI:
		return "SMI";
		break;
	 case HI_FORMAT_SUBTITLE_SUB:
		return "SUB";
		break;
	case HI_FORMAT_SUBTITLE_TXT:
		return "TEXT";
		break;
	case HI_FORMAT_SUBTITLE_HDMV_PGS:
		return "HDMV_PGS";
		break;
	case HI_FORMAT_SUBTITLE_DVB_SUB:
		return "DVB_SUB_BMP";
		break;
	case HI_FORMAT_SUBTITLE_DVD_SUB:
		return "DVD_SUB_BMP";
		break;
	default:
		return "UNKNOWN";
		break;
	}

	return "UNKNOWN";
}

std::string eServiceHisilicon::getInfoString(int w)
{
	HI_FORMAT_PROGRAM_INFO_S *pstProgram = NULL;
	HI_FORMAT_VID_INFO_S *pstVidStream = NULL;
	HI_FORMAT_AUD_INFO_S *pstAudStream = NULL;
	if (fileinfo.u32ProgramNum > 0 && streamid.programid < fileinfo.u32ProgramNum)
	{
		pstProgram = &fileinfo.pastProgramInfo[streamid.programid];
		if (fileinfo.pastProgramInfo[streamid.programid].u32VidStreamNum > 0)
		{
			pstVidStream = &fileinfo.pastProgramInfo[streamid.programid].pastVidStream[streamid.videostreamid];
		}
		if (fileinfo.pastProgramInfo[streamid.programid].u32AudStreamNum > 0)
		{
			pstAudStream = &fileinfo.pastProgramInfo[streamid.programid].pastAudStream[streamid.audiostreamid];
		}
	}

	switch (w)
	{
	case sProvider:
		if (pstProgram)
		{
			return pstProgram->aszServiceProvider;
		}
		break;
	case sServiceref:
	{
		eServiceReference ref(m_ref);
		ref.type = eServiceFactoryHisilicon::id;
		ref.path.clear();
		return ref.toString();
	}
	case sTagTitle:
		if (pstProgram)
		{
			return pstProgram->aszServiceName;
		}
		break;
	case sTagArtist:
		break;
	case sTagAlbum:
		break;
	case sTagTitleSortname:
		break;
	case sTagArtistSortname:
		break;
	case sTagAlbumSortname:
		break;
	case sTagDate:
		break;
	case sTagComposer:
		break;
	case sTagGenre:
		break;
	case sTagComment:
		break;
	case sTagExtendedComment:
		break;
	case sTagLocation:
		break;
	case sTagHomepage:
		break;
	case sTagDescription:
		break;
	case sTagVersion:
		break;
	case sTagISRC:
		break;
	case sTagOrganization:
		break;
	case sTagCopyright:
		break;
	case sTagCopyrightURI:
		break;
	case sTagContact:
		break;
	case sTagLicense:
		break;
	case sTagLicenseURI:
		break;
	case sTagCodec:
		break;
	case sTagAudioCodec:
		if (pstAudStream)
		{
			return getAudFormatStr(pstAudStream->u32Format);
		}
		break;
	case sTagVideoCodec:
		if (pstVidStream)
		{
			return getVidFormatStr(pstVidStream->u32Format);
		}
		break;
	case sTagEncoder:
		break;
	case sTagLanguageCode:
		if (pstAudStream)
		{
			return pstAudStream->aszLanguage;
		}
		break;
	case sTagKeywords:
		break;
	case sTagChannelMode:
		break;
	case sUser+12:
		return m_errorInfo.error_message;
	default:
		return "";
	}
	return "";
}

ePtr<iServiceInfoContainer> eServiceHisilicon::getInfoObject(int w)
{
	eServiceHisiliconInfoContainer *container = new eServiceHisiliconInfoContainer;
	ePtr<iServiceInfoContainer> retval = container;

	bool isBuffer = false;
	switch (w)
	{
		case sTagTrackGain:
			break;
		case sTagTrackPeak:
			break;
		case sTagAlbumGain:
			break;
		case sTagAlbumPeak:
			break;
		case sTagReferenceLevel:
			break;
		case sTagBeatsPerMinute:
			break;
		case sTagImage:
			/* TODO? */
			//isBuffer = true;
			break;
		case sTagPreviewImage:
			/* TODO? */
			//isBuffer = true;
			break;
		case sTagAttachment:
			/* TODO? */
			//isBuffer = true;
			break;
		default:
			break;
	}

	return retval;
}

RESULT eServiceHisilicon::audioChannel(ePtr<iAudioChannelSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceHisilicon::audioTracks(ePtr<iAudioTrackSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceHisilicon::cueSheet(ePtr<iCueSheet> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceHisilicon::subtitle(ePtr<iSubtitleOutput> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceHisilicon::audioDelay(ePtr<iAudioDelay> &ptr)
{
	ptr = this;
	return 0;
}

int eServiceHisilicon::getNumberOfTracks()
{
	HI_FORMAT_PROGRAM_INFO_S *pstProgram = NULL;
	if (fileinfo.u32ProgramNum > 0 && streamid.programid < fileinfo.u32ProgramNum)
	{
		pstProgram = &fileinfo.pastProgramInfo[streamid.programid];
	}
	return pstProgram ? pstProgram->u32AudStreamNum : 1;
}

int eServiceHisilicon::getCurrentTrack()
{
	return streamid.audiostreamid;
}

RESULT eServiceHisilicon::selectTrack(unsigned int i)
{
	bool validposition = false;
	pts_t ppos = 0;
	if (getPlayPosition(ppos) >= 0)
	{
		validposition = true;
		ppos -= 90000;
		if (ppos < 0)
			ppos = 0;
	}
	if (validposition)
	{
		/* flush */
		seekTo(ppos);
	}
	return selectAudioStream(i);
}

int eServiceHisilicon::selectAudioStream(int i)
{
	if (m_video_fd >= 0)
	{
		struct video_command cmd = {0};
		cmd.cmd = 105; /* set audio streamid */
		cmd.raw.data[0] = i;
		::ioctl(m_video_fd, VIDEO_COMMAND, &cmd);
	}
	return 0;
}

int eServiceHisilicon::getCurrentChannel()
{
	return STEREO;
}

RESULT eServiceHisilicon::selectChannel(int i)
{
	eDebug("[eServiceHisilicon] selectChannel(%i)",i);
	return 0;
}

RESULT eServiceHisilicon::getTrackInfo(struct iAudioTrackInfo &info, unsigned int i)
{
	HI_FORMAT_AUD_INFO_S *pstAudStream = NULL;
	if (fileinfo.u32ProgramNum > 0 && streamid.programid < fileinfo.u32ProgramNum)
	{
		if (fileinfo.pastProgramInfo[streamid.programid].u32AudStreamNum > i)
		{
			pstAudStream = &fileinfo.pastProgramInfo[streamid.programid].pastAudStream[i];
		}
	}

	if (!pstAudStream)
	{
		return -2;
	}

	info.m_description = getAudFormatStr(pstAudStream->u32Format);
	
	if (info.m_language.empty())
	{
		info.m_language = pstAudStream->aszLanguage;
	}
	return 0;
}

eAutoInitPtr<eServiceFactoryHisilicon> init_eServiceFactoryHisilicon(eAutoInitNumbers::service+2, "eServiceFactoryHisilicon");

RESULT eServiceHisilicon::enableSubtitles(iSubtitleUser *user, struct SubtitleTrack &track)
{
	if (m_currentSubtitleStream != track.pid)
	{
		m_prev_decoder_time = -1;
		m_decoder_time_valid_state = 0;

		if (m_video_fd >= 0)
		{
			struct video_command cmd = {0};
			cmd.cmd = 106; /* set subtitle streamid */
			cmd.raw.data[0] = track.pid;
			::ioctl(m_video_fd, VIDEO_COMMAND, &cmd);
		}

		m_subtitle_widget = user;

		m_currentSubtitleStream = track.pid;
		m_cachedSubtitleStream = m_currentSubtitleStream;

		eDebug("[eServiceHisilicon] switched to subtitle stream %i", m_currentSubtitleStream);
	}

	return 0;
}

RESULT eServiceHisilicon::disableSubtitles()
{
	eDebug("[eServiceHisilicon] disableSubtitles");
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = m_currentSubtitleStream;
	/* TODO: can we actually disable the subtitle output? */
	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	if (m_subtitle_widget) m_subtitle_widget->destroy();
	m_subtitle_widget = 0;
	return 0;
}

RESULT eServiceHisilicon::getCachedSubtitle(struct SubtitleTrack &track)
{

	bool autoturnon = eConfigManager::getConfigBoolValue("config.subtitles.pango_autoturnon", true);
	int m_subtitleStreams_size = (int)m_subtitleStreams.size();
	if (!autoturnon)
		return -1;

	if (m_cachedSubtitleStream == -2 && m_subtitleStreams_size)
	{
		m_cachedSubtitleStream = 0;
		int autosub_level = 5;
		std::string configvalue;
		std::vector<std::string> autosub_languages;
		configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect1");
		if (configvalue != "" && configvalue != "None")
			autosub_languages.push_back(configvalue);
		configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect2");
		if (configvalue != "" && configvalue != "None")
			autosub_languages.push_back(configvalue);
		configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect3");
		if (configvalue != "" && configvalue != "None")
			autosub_languages.push_back(configvalue);
		configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect4");
		if (configvalue != "" && configvalue != "None")
			autosub_languages.push_back(configvalue);
		for (int i = 0; i < m_subtitleStreams_size; i++)
		{
			if (!m_subtitleStreams[i].language_code.empty())
			{
				int x = 1;
				for (std::vector<std::string>::iterator it2 = autosub_languages.begin(); x < autosub_level && it2 != autosub_languages.end(); x++, it2++)
				{
					if ((*it2).find(m_subtitleStreams[i].language_code) != std::string::npos)
					{
						autosub_level = x;
						m_cachedSubtitleStream = i;
						break;
					}
				}
			}
		}
	}

	if (m_cachedSubtitleStream >= 0 && m_cachedSubtitleStream < m_subtitleStreams_size)
	{
		track.type = 2;
		track.pid = m_cachedSubtitleStream;
		track.page_number = int(m_subtitleStreams[m_cachedSubtitleStream].type);
		track.magazine_number = 0;
		return 0;
	}
	return -1;
}

RESULT eServiceHisilicon::getSubtitleList(std::vector<struct SubtitleTrack> &subtitlelist)
{
	if (fileinfo.u32ProgramNum > 0 && streamid.programid < fileinfo.u32ProgramNum)
	{
		unsigned int i;
		for (i = 0; i < fileinfo.pastProgramInfo[streamid.programid].u32SubStreamNum; i++)
		{
			struct SubtitleTrack track;
			track.type = 2;
			track.pid = i;
			switch (fileinfo.pastProgramInfo[streamid.programid].pastSubStream[i].u32Format)
			{
			case HI_FORMAT_SUBTITLE_ASS:
				track.page_number = 3; /* ASS */
				break;
			case HI_FORMAT_SUBTITLE_SRT:
				track.page_number = 4; /* SRT */
				break;
			case HI_FORMAT_SUBTITLE_HDMV_PGS:
				track.page_number = 6; /* PGS */
				break;
			case HI_FORMAT_SUBTITLE_DVD_SUB:
				track.page_number = 5; /* VOB */
				break;
			case HI_FORMAT_SUBTITLE_DVB_SUB: /* should not happen in ES media */
			case HI_FORMAT_SUBTITLE_LRC:
			case HI_FORMAT_SUBTITLE_SMI:
			case HI_FORMAT_SUBTITLE_SUB:
			case HI_FORMAT_SUBTITLE_TXT:
			default:
				track.page_number = fileinfo.pastProgramInfo[streamid.programid].pastSubStream[i].bExtSub ? 0 : 1; /* unknown / embedded */
				break;
			}
			track.magazine_number = 0;
			track.language_code = (const char*)fileinfo.pastProgramInfo[streamid.programid].pastSubStream[i].paszLanguage;
			subtitlelist.push_back(track);
		}
	}

	return 0;
}

RESULT eServiceHisilicon::streamed(ePtr<iStreamedService> &ptr)
{
	ptr = this;
	return 0;
}

ePtr<iStreamBufferInfo> eServiceHisilicon::getBufferCharge()
{
	/* TODO: get inrate / outrate / size / total */
	return new eHisiliconBufferInfo(m_bufferpercentage, 0, 0, 0, 0);
}

/* cuesheet CVR */
PyObject *eServiceHisilicon::getCutList()
{
	ePyObject list = PyList_New(0);

	for (std::multiset<struct cueEntry>::iterator i(m_cue_entries.begin()); i != m_cue_entries.end(); ++i)
	{
		ePyObject tuple = PyTuple_New(2);
		PyTuple_SET_ITEM(tuple, 0, PyLong_FromLongLong(i->where));
		PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(i->what));
		PyList_Append(list, tuple);
		Py_DECREF(tuple);
	}

	return list;
}
/* cuesheet CVR */
void eServiceHisilicon::setCutList(ePyObject list)
{
	if (!PyList_Check(list))
		return;
	int size = PyList_Size(list);
	int i;

	m_cue_entries.clear();

	for (i=0; i<size; ++i)
	{
		ePyObject tuple = PyList_GET_ITEM(list, i);
		if (!PyTuple_Check(tuple))
		{
			eDebug("[eServiceHisilicon] non-tuple in cutlist");
			continue;
		}
		if (PyTuple_Size(tuple) != 2)
		{
			eDebug("[eServiceHisilicon] cutlist entries need to be a 2-tuple");
			continue;
		}
		ePyObject ppts = PyTuple_GET_ITEM(tuple, 0), ptype = PyTuple_GET_ITEM(tuple, 1);
		if (!(PyLong_Check(ppts) && PyInt_Check(ptype)))
		{
			eDebug("[eServiceHisilicon] cutlist entries need to be (pts, type)-tuples (%d %d)", PyLong_Check(ppts), PyInt_Check(ptype));
			continue;
		}
		pts_t pts = PyLong_AsLongLong(ppts);
		int type = PyInt_AsLong(ptype);
		m_cue_entries.insert(cueEntry(pts, type));
		eDebug("[eServiceHisilicon] adding %08llx, %d", pts, type);
	}
	m_cuesheet_changed = 1;
	m_event((iPlayableService*)this, evCuesheetChanged);
}

void eServiceHisilicon::setCutListEnable(int enable)
{
	m_cutlist_enabled = enable;
}

int eServiceHisilicon::setBufferSize(int size)
{
	return 0;
}

int eServiceHisilicon::getAC3Delay()
{
	return ac3_delay;
}

int eServiceHisilicon::getPCMDelay()
{
	return pcm_delay;
}

void eServiceHisilicon::setAC3Delay(int delay)
{
	ac3_delay = delay;
	if (m_state != stRunning)
		return;
	else
	{
		int config_delay_int = delay;

		config_delay_int += eConfigManager::getConfigIntValue("config.av.generalAC3delay");
		eTSMPEGDecoder::setHwAC3Delay(config_delay_int);
	}
}

void eServiceHisilicon::setPCMDelay(int delay)
{
	pcm_delay = delay;
	if (m_state != stRunning)
		return;
	else
	{
		int config_delay_int = delay;
		config_delay_int += eConfigManager::getConfigIntValue("config.av.generalPCMdelay");
		eTSMPEGDecoder::setHwPCMDelay(config_delay_int);
	}
}
/* cuesheet CVR */
void eServiceHisilicon::loadCuesheet()
{
	if (!m_cuesheet_loaded)
	{
		eDebug("[eServiceHisilicon] loading cuesheet");
		m_cuesheet_loaded = true;
	}
	else
	{
		eDebug("[eServiceHisilicon] skip loading cuesheet multiple times");
		return;
	}
 
	m_cue_entries.clear();
	/* only load manual cuts if no chapter info avbl CVR */

	std::string filename = m_ref.path + ".cuts";

	m_cue_entries.clear();

	FILE *f = fopen(filename.c_str(), "rb");

	if (f)
	{
		while (1)
		{
			unsigned long long where;
			unsigned int what;

			if (!fread(&where, sizeof(where), 1, f))
				break;
			if (!fread(&what, sizeof(what), 1, f))
				break;

			where = be64toh(where);
			what = ntohl(what);

			if (what > 3)
				break;

			m_cue_entries.insert(cueEntry(where, what));
		}
		fclose(f);
		eDebug("[eServiceHisilicon] cuts file has %zd entries", m_cue_entries.size());
	} else
		eDebug("[eServiceHisilicon] cutfile not found!");

	m_cuesheet_changed = 0;
	m_event((iPlayableService*)this, evCuesheetChanged);
}
/* cuesheet CVR */
void eServiceHisilicon::saveCuesheet()
{
	std::string filename = m_ref.path;

	/* save cuesheet only when main file is accessible. */
	if (::access(filename.c_str(), R_OK) < 0)
		return;
	filename.append(".cuts");
	/* do not save to file if there are no cuts */
	/* remove the cuts file if cue is empty */
	if(m_cue_entries.begin() == m_cue_entries.end())
	{
		if (::access(filename.c_str(), F_OK) == 0)
			remove(filename.c_str());
		return;
	}

	FILE *f = fopen(filename.c_str(), "wb");

	if (f)
	{
		unsigned long long where;
		int what;

		for (std::multiset<cueEntry>::iterator i(m_cue_entries.begin()); i != m_cue_entries.end(); ++i)
		{
			where = htobe64(i->where);
			what = htonl(i->what);
			fwrite(&where, sizeof(where), 1, f);
			fwrite(&what, sizeof(what), 1, f);

		}
		fclose(f);
	}
	m_cuesheet_changed = 0;
}
