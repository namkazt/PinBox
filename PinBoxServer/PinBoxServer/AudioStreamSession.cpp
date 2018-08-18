#include "stdafx.h"
#include "AudioStreamSession.h"
#include <iostream>
#include <cassert>
#include "const.h"

namespace
{
	void createNew(void* arg) {
		static_cast<AudioStreamSession*>(arg)->loopbackThread();
	}
}

AudioStreamSession::~AudioStreamSession()
{
	if (m_pMMDevice != NULL) m_pMMDevice->Release();
}

void AudioStreamSession::useDefaultDevice()
{
	HRESULT hr = S_OK;
	IMMDeviceEnumerator *pMMDeviceEnumerator;
	//------------------------------------------------
	// activate a device enumerator
	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pMMDeviceEnumerator
	);
	if (FAILED(hr)) {
		printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
		return;
	}
	//------------------------------------------------
	// get the default render endpoint
	hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pMMDevice);
	if (FAILED(hr)) {
		printf("IMMDeviceEnumerator::GetDefaultAudioEndpoint failed: hr = 0x%08x", hr);
	}
	pMMDeviceEnumerator->Release();
}

void AudioStreamSession::StartAudioStream()
{
	CoInitialize(NULL);
	this->useDefaultDevice();
	//-----------------------------------------------------
	// create a "stop capturing now" event
	g_StopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == g_StopEvent) {
		printf("CreateEvent failed: last error is %u", GetLastError());
		return;
	}
	//-----------------------------------------------------
	// init tmp buffer
	audioBuffer = (u8*)malloc(5242880);
	audioBufferSize = 0;
	audioFrames = 0;
	mutex = new std::mutex();
	//-----------------------------------------------------
	// start loopback record thread
	g_thread = std::thread(createNew, static_cast<AudioStreamSession*>(this));
}


void AudioStreamSession::StopStreaming()
{
	SetEvent(g_StopEvent);

	if (m_pMMDevice != NULL) m_pMMDevice->Release();
	m_pMMDevice = NULL;
	if (audioBuffer) free(audioBuffer);
}

void AudioStreamSession::ReadFromBuffer(u8* outBuf, u32 size)
{
	if (!outBuf) return;
	if (size > audioBufferSize) return;
	mutex->lock();
	// read memory to buf
	memcpy(outBuf, audioBuffer, size);

	// update pointer cursor
	audioBufferSize -= size;
	// shift memory back to place
	memcpy(audioBuffer, audioBuffer + size, audioBufferSize);
	mutex->unlock();
}


void AudioStreamSession::Pause()
{
	if (g_isPaused) return;
	g_isPaused = true;
	HRESULT hr = S_OK;
	hr = pAudioClient->Stop();
	if (FAILED(hr)) {
		printf("IAudioClient::Stop failed: hr = 0x%08x\n", hr);
		return;
	}
}

void AudioStreamSession::Resume()
{
	if (!g_isPaused) return;
	g_isPaused = false;
	HRESULT hr = S_OK;
	hr = pAudioClient->Start();
	if (FAILED(hr)) {
		printf("IAudioClient::Start failed: hr = 0x%08x\n", hr);
		return;
	}
}

void AudioStreamSession::loopbackThread()
{
	HRESULT hr = S_OK;
	hr = m_pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
	if (FAILED(hr)) {
		printf("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x\n", hr);
		return;
	}

	// get the default device periodicity
	REFERENCE_TIME hnsDefaultDevicePeriod;
	hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
	if (FAILED(hr)) {
		printf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x\n", hr);
		return;
	}

	// get the default device format
	WAVEFORMATEX *pwfx;
	hr = pAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr)) {
		printf("IAudioClient::GetMixFormat failed: hr = 0x%08x\n", hr);
		return;
	}

	//pwfx->nSamplesPerSec = 22050;

	switch (pwfx->wFormatTag) {
	case WAVE_FORMAT_IEEE_FLOAT:
		pwfx->wFormatTag = WAVE_FORMAT_PCM;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
		// naked scope for case-local variable
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			pEx->Samples.wValidBitsPerSample = 16;
			pwfx->wBitsPerSample = 16; // 2 bytes per sample
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
		}
		else {
			printf("Don't know how to coerce mix format to int-16\n");
			return;
		}
		break;

	}

	pwfx->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

	//pwfx->cbSize = 0;
	// create a periodic waitable timer
	HANDLE hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
	UINT32 nBlockAlign = pwfx->nBlockAlign;

	// call IAudioClient::Initialize
	// note that AUDCLNT_STREAMFLAGS_LOOPBACK and AUDCLNT_STREAMFLAGS_EVENTCALLBACK
	// do not work together...
	// the "data ready" event never gets set
	// so we're going to do a timer-driven loop
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,  // try this ? AUDCLNT_SHAREMODE_EXCLUSIVE
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		0, 0, pwfx, 0
	);
	if (FAILED(hr)) {
		printf(">>>>> Error: IAudioClient::Initialize failed: hr = 0x%08x\n", hr);
		return;
	}

	sampleRate = (u32)pwfx->nSamplesPerSec;

	// activate an IAudioCaptureClient
	IAudioCaptureClient *pAudioCaptureClient;
	hr = pAudioClient->GetService( __uuidof(IAudioCaptureClient), (void**)&pAudioCaptureClient );

	// set the waitable timer
	LARGE_INTEGER liFirstFire;
	liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2; // negative means relative time
	LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds
	BOOL bOK = SetWaitableTimer(
		hWakeUp,
		&liFirstFire,
		lTimeBetweenFires,
		NULL, NULL, FALSE
	);

	// call IAudioClient::Start
	hr = pAudioClient->Start();
	
	if (FAILED(hr)) {
		printf("IAudioClient::Start failed: hr = 0x%08x\n", hr);
		return;
	}

	// loopback capture loop
	HANDLE waitArray[2] = { g_StopEvent, hWakeUp };
	DWORD dwWaitResult;


	bool bDone = false;
	for (UINT32 nPasses = 0; !bDone; nPasses++) {

		// drain data while it is available
		UINT32 nNextPacketSize;
		for (hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize); SUCCEEDED(hr) && nNextPacketSize > 0; hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize))
		{
			// get the captured data
			BYTE *pData;
			UINT32 nNumFramesToRead;
			DWORD dwFlags;

			hr = pAudioCaptureClient->GetBuffer(
				&pData,
				&nNumFramesToRead,
				&dwFlags,
				NULL,
				NULL
			);


			if(dwFlags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				pData = NULL;
				//TODO: send data to write as silent ?
				continue;
			}

			LONG lBytesToWrite = nNumFramesToRead * nBlockAlign;
			// write audio wave data into tmp buffer to wait for encode
			mutex->lock();
			audioFrames += nNumFramesToRead;
			if(audioBufferSize + lBytesToWrite < 5242880)
			{
				memcpy(audioBuffer + audioBufferSize, pData, lBytesToWrite);
				audioBufferSize += lBytesToWrite;
			}
			mutex->unlock();


			hr = pAudioCaptureClient->ReleaseBuffer(nNumFramesToRead);
		}

		//-------------------------------------------------------------------------
		// event result if have
		
		dwWaitResult = WaitForMultipleObjects(
			ARRAYSIZE(waitArray), waitArray,
			FALSE, INFINITE
		);
		if (WAIT_OBJECT_0 == dwWaitResult) {
			printf("Received stop event after %u passes\n", nPasses);
			bDone = true;
			continue; // exits loop
		}
		if (WAIT_OBJECT_0 + 1 != dwWaitResult) {
			printf("Unexpected WaitForMultipleObjects return value %u on pass %u\n", dwWaitResult, nPasses);
			return;
		}
	}

	//---------------------------------------
	// finish record data 
	// need send something here to end that stream
	//---------------------------------------
	pAudioClient->Stop();
	CancelWaitableTimer(hWakeUp);
	pAudioCaptureClient->Release();
	CloseHandle(hWakeUp);
	CoTaskMemFree(pwfx);
	pAudioClient->Release();
	CloseHandle(g_StopEvent);
	CoUninitialize();


}
