// https://github.com/thestk/rtmidi/blob/master/RtMidi.cpp#L2850

#include <exception>
#include <stdexcept>


#define NOMINMAX
#include <windows.h>


namespace WindowsMultiMedia
{


class MooerMidiControl
{
public:
	MooerMidiControl(int portNumber);

	~MooerMidiControl();

	void OnProcess(UINT inputStatus, DWORD timestamp, DWORD_PTR midiMessage);

private:
	HMIDIIN m_inputPort;
};


static void CALLBACK
midiInputCallback(HMIDIIN /*hmin*/, UINT inputStatus, DWORD_PTR instancePtr, DWORD_PTR midiMessage, DWORD timestamp)
{
	return reinterpret_cast<MooerMidiControl*>(instancePtr)->OnProcess(inputStatus, timestamp, midiMessage);
}


MooerMidiControl::MooerMidiControl(int portNumber)
{
	MMRESULT result =
		midiInOpen(&m_inputPort, portNumber, (DWORD_PTR)&midiInputCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if(result != MMSYSERR_NOERROR)
		throw std::runtime_error("Could not open Midi device");


	// midiInPrepareHeader

	// result = midiInAddBuffer( data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR) );

	// result = midiInStart( data->inHandle );
}


MooerMidiControl::~MooerMidiControl()
{
	midiInReset(m_inputPort);
	midiInStop(m_inputPort);
	// int result = midiInUnprepareHeader(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
	midiInClose(m_inputPort);
}


void MooerMidiControl::OnProcess(UINT inputStatus, DWORD timestamp, DWORD_PTR midiMessage)
{
	unsigned char status = (unsigned char)(midiMessage & 0x000000FF);
	if(!(status & 0x80))
		return; // This is not a status byte

	if(inputStatus == MIM_DATA)
	{ // Channel or system message
	}
	else
	{
		// Sysex message ( MIM_LONGDATA or MIM_LONGERROR )
	}
}



} // namespace WindowsMultiMedia
