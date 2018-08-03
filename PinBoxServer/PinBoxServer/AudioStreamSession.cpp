#include "stdafx.h"
#include "AudioStreamSession.h"
#include <iostream>


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
	mTmpAudioBuffer = (u8*)malloc(5 * 1024 * 1024);
	mTmpAudioBufferSize = 0;
	mutex = new std::mutex();
	//-----------------------------------------------------
	// start loopback record thread
	g_thread = std::thread(loopbackCaptureThreadFunction, this);
	g_thread.detach();
}

void AudioStreamSession::StopStreaming()
{
	
	if (m_pMMDevice != NULL) m_pMMDevice->Release();
	m_pMMDevice = NULL;
	if (mTmpAudioBuffer) free(mTmpAudioBuffer);
}


void AudioStreamSession::BeginReadAudioBuffer()
{
	mutex->lock();
}

int AudioStreamSession::FinishReadAudioBuffer(u32 sizeRead, int framesRead)
{
	if (mTmpAudioBufferSize < sizeRead) sizeRead = mTmpAudioBufferSize;
	if (mTmpAudioFrames < framesRead) framesRead = mTmpAudioFrames;
	// move data left
	//std::cout << "Memory read: " << sizeRead << " frames read: " << framesRead << " --- memory left: " << mTmpAudioBufferSize - sizeRead << std::endl << std::flush;
	memmove(mTmpAudioBuffer, mTmpAudioBuffer + sizeRead, mTmpAudioBufferSize - sizeRead);
	// clean data
	mTmpAudioBufferSize -= sizeRead;
	mTmpAudioFrames -= framesRead;
	mutex->unlock();
	return mTmpAudioFrames;
}

void AudioStreamSession::loopbackCaptureThreadFunction(void* context)
{
	AudioStreamSession* self = (AudioStreamSession*)context;
	//------------------------------------------------------------------
	HRESULT hr = S_OK;
	//------------------------------------------------------------------
	// loop back capture
	//------------------------------------------------------------------
	IAudioClient *pAudioClient;
	hr = self->m_pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
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

	// int-16 wave format ( if use it )
	if(true)
	{
		switch (pwfx->wFormatTag) {
		case WAVE_FORMAT_IEEE_FLOAT:
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->wBitsPerSample = 16;
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			break;

		case WAVE_FORMAT_EXTENSIBLE:
		{
			// naked scope for case-local variable
			PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
			if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
				pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
				pEx->Samples.wValidBitsPerSample = 16;
				pwfx->wBitsPerSample = 16;
				pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
				pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			}
			else {
				printf("Don't know how to coerce mix format to int-16\n");
				return;
			}
		}
		break;

		default:
			printf("Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to int-16\n", pwfx->wFormatTag);
			return;
		}
	}

	//NOTE: can be create as event to get on another thread
	//callback for device information
	//if(self->g_opus_onGetDeviceInfo!= nullptr)
	//	self->g_opus_onGetDeviceInfo(pwfx->nChannels, pwfx->nSamplesPerSec);

	//NOTE: test result wave file
	//MMCKINFO ckRIFF = { 0 };
	//MMCKINFO ckData = { 0 };
	//self->Helper_WriteWaveHeader(L"test.wav", pwfx, &ckRIFF, &ckData);


	// create a periodic waitable timer
	HANDLE hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
	UINT32 nBlockAlign = pwfx->nBlockAlign;
	self->g_totalFrameRecorded = 0;

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
	HANDLE waitArray[2] = { self->g_StopEvent, hWakeUp };
	DWORD dwWaitResult;


	bool bDone = false;
	bool bFirstPacket = true;
	for (UINT32 nPasses = 0; !bDone; nPasses++) {

		if(self->g_isPaused)
		{
			continue;
		}

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

			LONG lBytesToWrite = nNumFramesToRead * nBlockAlign;

			// write audio wave data into tmp buffer to wait for encode
			self->mutex->lock();
			memmove(self->mTmpAudioBuffer + self->mTmpAudioBufferSize, pData, lBytesToWrite);
			self->mTmpAudioBufferSize += lBytesToWrite;
			self->mTmpAudioFrames += nNumFramesToRead;
			//std::cout << "Add: " << lBytesToWrite << " frames: " << nNumFramesToRead << " --- memory new: " << self->mTmpAudioBufferSize << std::endl << std::flush;
			self->mutex->unlock();
			//-------------------------------------------------------------------------
			self->g_totalFrameRecorded += nNumFramesToRead;
			hr = pAudioCaptureClient->ReleaseBuffer(nNumFramesToRead);
			bFirstPacket = false;
		}

		//-------------------------------------------------------------------------
		// event result if have
		
		dwWaitResult = WaitForMultipleObjects(
			ARRAYSIZE(waitArray), waitArray,
			FALSE, INFINITE
		);
		if (WAIT_OBJECT_0 == dwWaitResult) {
			printf("Received stop event after %u passes and %u frames\n", nPasses, self->g_totalFrameRecorded);
			bDone = true;
			continue; // exits loop
		}
		if (WAIT_OBJECT_0 + 1 != dwWaitResult) {
			printf("Unexpected WaitForMultipleObjects return value %u on pass %u after %u frames\n", dwWaitResult, nPasses, self->g_totalFrameRecorded);
			return;
		}
	}

	//NOTE: test write wave file
	//self->Helper_FinishWaveFile(L"test.wav", &ckData, &ckRIFF);

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
	CloseHandle(self->g_StopEvent);
	CoUninitialize();
}


//---------------------------------------
// Those functions only for testing
//---------------------------------------


void AudioStreamSession::Helper_WriteWaveHeader(LPCWSTR fileName, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
	MMRESULT result;

	g_tmpWavFile = mmioOpen(const_cast<LPWSTR>(fileName), nullptr, MMIO_WRITE | MMIO_CREATE);

	// make a RIFF/WAVE chunk
	pckRIFF->ckid = MAKEFOURCC('R', 'I', 'F', 'F');
	pckRIFF->fccType = MAKEFOURCC('W', 'A', 'V', 'E');

	result = mmioCreateChunk(g_tmpWavFile, pckRIFF, MMIO_CREATERIFF);
	if (MMSYSERR_NOERROR != result) {
		return;
	}

	// make a 'fmt ' chunk (within the RIFF/WAVE chunk)
	MMCKINFO chunk;
	chunk.ckid = MAKEFOURCC('f', 'm', 't', ' ');
	result = mmioCreateChunk(g_tmpWavFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}

	// write the WAVEFORMATEX data to it
	LONG lBytesInWfx = sizeof(WAVEFORMATEX) + pwfx->cbSize;
	LONG lBytesWritten =
		mmioWrite(
			g_tmpWavFile,
			reinterpret_cast<PCHAR>(const_cast<LPWAVEFORMATEX>(pwfx)),
			lBytesInWfx
		);
	if (lBytesWritten != lBytesInWfx) {
		return;
	}

	// ascend from the 'fmt ' chunk
	result = mmioAscend(g_tmpWavFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}

	// make a 'fact' chunk whose data is (DWORD)0
	chunk.ckid = MAKEFOURCC('f', 'a', 'c', 't');
	result = mmioCreateChunk(g_tmpWavFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}

	// write (DWORD)0 to it
	// this is cleaned up later
	DWORD frames = 0;
	lBytesWritten = mmioWrite(g_tmpWavFile, reinterpret_cast<PCHAR>(&frames), sizeof(frames));
	if (lBytesWritten != sizeof(frames)) {
		return;
	}

	// ascend from the 'fact' chunk
	result = mmioAscend(g_tmpWavFile, &chunk, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}

	// make a 'data' chunk and leave the data pointer there
	pckData->ckid = MAKEFOURCC('d', 'a', 't', 'a');
	result = mmioCreateChunk(g_tmpWavFile, pckData, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}
}

void AudioStreamSession::Helper_FinishWaveFile(LPCWSTR fileName, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
	MMRESULT result;
	result = mmioAscend(g_tmpWavFile, pckData, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}
	result = mmioAscend(g_tmpWavFile, pckRIFF, 0);
	if (MMSYSERR_NOERROR != result) {
		return;
	}
	result = mmioClose(g_tmpWavFile, 0);

	Helper_FixWaveFile(fileName, g_totalFrameRecorded);
}

void AudioStreamSession::Helper_FixWaveFile(LPCWSTR fileName, u32 nFrames)
{
	MMRESULT result;
	// reopen the file in read/write mode
	MMIOINFO mi = { 0 };
	HMMIO hFile = mmioOpen(const_cast<LPWSTR>(fileName), &mi, MMIO_READWRITE);

	// descend into the RIFF/WAVE chunk
	MMCKINFO ckRIFF = { 0 };
	ckRIFF.ckid = MAKEFOURCC('W', 'A', 'V', 'E'); // this is right for mmioDescend
	result = mmioDescend(hFile, &ckRIFF, NULL, MMIO_FINDRIFF);

	// descend into the fact chunk
	MMCKINFO ckFact = { 0 };
	ckFact.ckid = MAKEFOURCC('f', 'a', 'c', 't');
	result = mmioDescend(hFile, &ckFact, &ckRIFF, MMIO_FINDCHUNK);

	// write the correct data to the fact chunk
	// write g_totalFrameRecorded
	LONG lBytesWritten = mmioWrite( hFile, reinterpret_cast<PCHAR>(&nFrames), sizeof(nFrames) );

	// ascend out of the fact chunk
	result = mmioAscend(hFile, &ckFact, 0);
	result = mmioClose(hFile, 0);
}