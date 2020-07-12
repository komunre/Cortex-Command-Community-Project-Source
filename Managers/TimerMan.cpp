#include "TimerMan.h"

namespace RTE {

	const std::string TimerMan::c_ClassName = "TimerMan";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void TimerMan::Clear() {
		m_StartTime = 0;
		m_TicksPerSecond = 1;
		m_RealTimeTicks = 0;
		m_RealToSimCap = 0;
		m_SimTimeTicks = 0;
		m_SimUpdateCount = 0;
		m_SimAccumulator = 0;
		m_DeltaTime = 0;
		m_DeltaTimeS = 0.016666666F;
		m_DeltaBuffer.clear();
		m_SimUpdatesSinceDrawn = -1;
		m_DrawnSimUpdate = false;
		m_TimeScale = 1.0F;
		m_AveragingEnabled = false;
		m_SimPaused = false;
		// This gets dynamically turned on for short periods when sim gets heavy (explosions) and slow-mo effect is appropriate
		m_OneSimUpdatePerFrame = false;
		m_SimSpeedLimited = true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int TimerMan::Create() {
		// Get the frequency of ticks/s for this machine
		LARGE_INTEGER tempLInt;
		QueryPerformanceFrequency(&tempLInt);
		m_TicksPerSecond = tempLInt.QuadPart;

		// Reset the real time setting so that we can measure how much real time has passed till the next Update.
		ResetTime();

		// Calculate a reasonable delta time in ticks, based on the now known frequency
		SetDeltaTimeSecs(m_DeltaTimeS);

		// Set up a default cap if one hasn't been set yet
		if (m_RealToSimCap <= 0) { m_RealToSimCap = 0.0333333F * m_TicksPerSecond; }

		return 0;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	long long TimerMan::GetAbsoulteTime() const {
		long long ticks;
		LARGE_INTEGER tickReading;

		QueryPerformanceCounter(&tickReading);

		ticks = tickReading.QuadPart;

		ticks *= 1000000;
		ticks /= m_TicksPerSecond;

		return ticks;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void TimerMan::ResetTime() {
		// Set the new starting point
		LARGE_INTEGER tempLInt;
		QueryPerformanceCounter(&tempLInt);
		m_StartTime = tempLInt.QuadPart;

		m_RealTimeTicks = 0;
		m_SimAccumulator = 0;
		m_SimTimeTicks = 0;
		m_SimUpdateCount = 0;
		m_SimUpdatesSinceDrawn = -1;
		m_DrawnSimUpdate = false;
		m_TimeScale = 1.0F;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void TimerMan::UpdateSim() {
		if (TimeForSimUpdate()) {
			// Transfer ticks from the accumulator to the sim time ticks
			m_SimAccumulator -= m_DeltaTime;
			m_SimTimeTicks += m_DeltaTime;
			// Increment the sim update count
			++m_SimUpdateCount;
			++m_SimUpdatesSinceDrawn;

			// If after deducting the DeltaTime from the Accumulator, there is not enough time for another DeltaTime, then flag this as the last sim update before the frame is drawn.
			m_DrawnSimUpdate = !TimeForSimUpdate();
		} else {
			m_DrawnSimUpdate = true;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void TimerMan::Update() {
		long long prevTime = m_RealTimeTicks;

		LARGE_INTEGER tickReading;

		// Increase the real time ticks with the amount of actual time passed since the last Update
		QueryPerformanceCounter(&tickReading);

		m_RealTimeTicks = tickReading.QuadPart - m_StartTime;

		// Figure the increase in real time 
		unsigned long long timeIncrease = m_RealTimeTicks - prevTime;
		// Cap it if too long (as when the app went out of focus)
		if (timeIncrease > m_RealToSimCap) { timeIncrease = m_RealToSimCap; }

		RTEAssert(timeIncrease > 0, "It seems your CPU is giving bad timing data to the game, this is known to happen on some multi-core processors. This may be fixed by downloading the latest CPU drivers from AMD or Intel.");

		// If not paused, add the new time difference to the sim accumulator, scaling by the TimeScale
		if (!m_SimPaused) { m_SimAccumulator += timeIncrease * m_TimeScale; }

		RTEAssert(m_SimAccumulator >= 0, "Negative sim time accumulator?!");

		// Reset the counter since the last drawn update. Set it negative since we're counting full pure sim updates and this will be incremented to 0 on next SimUpdate
		if (m_DrawnSimUpdate) { m_SimUpdatesSinceDrawn = -1; }

		// Override the accumulator and just put one delta time in there so sim updates only once per frame
		if (m_OneSimUpdatePerFrame) {
			// Only let it appear to go slower, not faster, if limited
			if (m_SimSpeedLimited && m_SimAccumulator > m_DeltaTime) { m_SimAccumulator = m_DeltaTime; }

			// Reset the counter of sim updates since the last drawn.. it will always be 0 since every update results in a drawn frame
			m_SimUpdatesSinceDrawn = -1;
		}
	}
}