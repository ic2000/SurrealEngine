
#include "Precomp.h"
#include "AudioSubsystem.h"
#include "AudioSource.h"
#include "UObject/UActor.h"
#include "UObject/UClient.h"
#include "UObject/ULevel.h"
#include "UObject/USound.h"
#include "UObject/UMusic.h"

AudioSubsystem::AudioSubsystem()
{
	Mixer = AudioMixer::Create();
}

void AudioSubsystem::SetViewport(UViewport* InViewport)
{
	if (Viewport != InViewport)
	{
		for (size_t i = 0; i < PlayingSounds.size(); i++)
			StopSound(i);

		if (Viewport)
		{
			Mixer->PlayMusic({});
		}

		Viewport = InViewport;

		if (Viewport)
		{
			if (Viewport->Actor()->Song() && Viewport->Actor()->Transition() == MTRAN_None)
				Viewport->Actor()->Transition() = MTRAN_Instant;

			PlayingSounds.resize(Channels);
		}
	}
}

UViewport* AudioSubsystem::GetViewport()
{
	return Viewport;
}

void AudioSubsystem::Update(const mat4& listener)
{
	StartAmbience();
	UpdateAmbience();
	UpdateSounds(listener);
	UpdateMusic();

	Mixer->SetMusicVolume(MusicVolume / 255.0f);
	Mixer->SetSoundVolume(SoundVolume / 255.0f);
	Mixer->Update();
}

static float distSquared(const vec3& a, const vec3& b)
{
	vec3 d = b - a;
	return dot(d, d);
}

static float square(float v)
{
	return v * v;
}

void AudioSubsystem::StartAmbience()
{
	bool Realtime = Viewport->IsRealtime() && Viewport->Actor()->Level()->Pauser() == "";
	if (Realtime)
	{
		UActor* ViewActor = Viewport->Actor()->ViewTarget() ? Viewport->Actor()->ViewTarget() : Viewport->Actor();
		int actorIndex = 0;
		for (UActor* Actor : Viewport->Actor()->XLevel()->Actors)
		{
			if (Actor && Actor->AmbientSound() && distSquared(ViewActor->Location(), Actor->Location()) <= square(Actor->WorldSoundRadius()))
			{
				int Id = actorIndex * 16 + SLOT_Ambient * 2;
				bool foundSound = false;
				for (size_t j = 0; j < PlayingSounds.size(); j++)
				{
					if (PlayingSounds[j].Id == Id)
					{
						foundSound = true;
						break;
					}
				}
				if (!foundSound)
					PlaySound(Actor, Id, Actor->AmbientSound(), Actor->Location(), AmbientFactor * Actor->SoundVolume() / 255.0f, Actor->WorldSoundRadius(), Actor->SoundPitch() / 64.0f);
			}
			actorIndex++;
		}
	}
}

void AudioSubsystem::UpdateAmbience()
{
	UActor* ViewActor = Viewport->Actor()->ViewTarget() ? Viewport->Actor()->ViewTarget() : Viewport->Actor();
	bool Realtime = Viewport->IsRealtime() && Viewport->Actor()->Level()->Pauser() == "";
	for (size_t i = 0; i < PlayingSounds.size(); i++)
	{
		PlayingSound& Playing = PlayingSounds[i];
		if ((Playing.Id & 14) == SLOT_Ambient * 2)
		{
			if (distSquared(ViewActor->Location(), Playing.Actor->Location()) > square(Playing.Actor->WorldSoundRadius()) || Playing.Actor->AmbientSound() != Playing.Sound || !Realtime)
			{
				// Ambient sound went out of range
				StopSound(i);
			}
			else
			{
				// Update basic sound properties
				Playing.Volume = 2.0f * (AmbientFactor * Playing.Actor->SoundVolume() / 255.0f);
				Playing.Radius = Playing.Actor->WorldSoundRadius();
				Playing.Pitch = Playing.Actor->SoundPitch() / 64.0f;
			}
		}
	}
}

void AudioSubsystem::UpdateSounds(const mat4& listener)
{
	UActor* ViewActor = Viewport->Actor()->ViewTarget() ? Viewport->Actor()->ViewTarget() : Viewport->Actor();
	for (size_t i = 0; i < PlayingSounds.size(); i++)
	{
		PlayingSound& Playing = PlayingSounds[i];

		if (Playing.Id == 0)
		{
			continue;
		}
		else if (Playing.Channel && Mixer->SoundFinished(Playing.Channel))
		{
			StopSound(i);
		}
		else
		{
			// Update positioning from actor, if available
			if (Playing.Actor)
				Playing.Location = Playing.Actor->Location();

			// Update the priority
			Playing.Priority = SoundPriority(Viewport, Playing.Location, Playing.Volume, Playing.Radius);

			// Compute the spatialization
			vec3 Location = (listener * vec4(Playing.Location, 1.0f)).xyz();
			float PanAngle = std::atan2(Location.x, std::abs(Location.z));

			// Despatialize sounds when you get real close to them
			float CenterDist = 0.1f * Playing.Radius;
			float Size = length(Location);
			if (Size < CenterDist)
				PanAngle *= Size / CenterDist;

			// Compute panning and volume
			float SoundPan = clamp(PanAngle * 7 / 8 / 3.14159265359f, -1.0f, 1.0f);
			float Attenuation = clamp(1.0f - Size / Playing.Radius, 0.0f, 1.0f);
			float SoundVolume = clamp(Playing.Volume * Attenuation, 0.0f, 1.0f);
			if (ReverseStereo)
				SoundPan = -SoundPan;

			// Compute doppler shifting
			float Doppler = 1.0f;
			if (Playing.Actor)
			{
				float V = dot(Playing.Actor->Velocity(), normalize(Playing.Actor->Location() - ViewActor->Location()));
				Doppler = clamp(1.0f - V / DopplerSpeed, 0.5f, 2.0f);
			}

			// Update the sound.
			AudioSound* Sound = Playing.Sound->GetSound();

			Playing.CurrentVolume = SoundVolume;
			if (Playing.Channel)
			{
				Mixer->UpdateSound(Playing.Channel, Sound, SoundVolume * 0.25f, SoundPan, Playing.Pitch * Doppler);
			}
			else
			{
				Playing.Channel = Mixer->PlaySound((int)i + 1, Sound, SoundVolume * 0.25f, SoundPan, Playing.Pitch * Doppler);
			}
		}
	}
}

void AudioSubsystem::UpdateMusic()
{
	if (Viewport->Actor() && Viewport->Actor()->Transition() != MTRAN_None)
	{
		// To do: this needs to fade out the old song before switching

		if (CurrentSong)
		{
			Mixer->PlayMusic({});
			CurrentSong = nullptr;
		}

		CurrentSong = Viewport->Actor()->Song();
		CurrentSection = Viewport->Actor()->SongSection();

		if (CurrentSong && UseDigitalMusic)
		{
			int subsong = CurrentSection != 255 ? CurrentSection : 0;
			Mixer->PlayMusic(AudioSource::CreateMod(CurrentSong->Data, true, 0, subsong));
		}

		Viewport->Actor()->Transition() = MTRAN_None;
	}
}

bool AudioSubsystem::PlaySound(UActor* Actor, int Id, USound* Sound, vec3 Location, float Volume, float Radius, float Pitch)
{
	if (!Viewport || !Sound)
		return false;

	// Allocate a new slot if requested
	if ((Id & 14) == 2 * SLOT_None)
		Id = 16 * --FreeSlot;

	float Priority = SoundPriority(Viewport, Location, Volume, Radius);

	// If already playing, stop it
	size_t Index = PlayingSounds.size();
	float BestPriority = Priority;
	for (size_t i = 0; i < PlayingSounds.size(); i++)
	{
		PlayingSound& Playing = PlayingSounds[i];
		if ((Playing.Id & ~1) == (Id & ~1))
		{
			// Skip if not interruptable.
			if (Id & 1)
				return 0;

			// Stop the sound.
			Index = i;
			break;
		}
		else if (Playing.Priority <= BestPriority)
		{
			Index = i;
			BestPriority = Playing.Priority;
		}
	}

	// If no sound, or its priority is overruled, stop it
	if (Index == PlayingSounds.size())
		return 0;

	// Put the sound on the play-list
	StopSound(Index);
	PlayingSounds[Index] = PlayingSound(Actor, Id, Sound, Location, Volume, Radius, Pitch, Priority);

	return true;
}

void AudioSubsystem::StopSound(size_t index)
{
	PlayingSound& Playing = PlayingSounds[index];

	if (Playing.Channel)
	{
		Mixer->StopSound(Playing.Channel);
	}

	PlayingSounds[index] = {};
}

void AudioSubsystem::NoteDestroy(UActor* Actor)
{
	for (size_t i = 0; i < PlayingSounds.size(); i++)
	{
		if (PlayingSounds[i].Actor == Actor)
		{
			if ((PlayingSounds[i].Id & 14) == SLOT_Ambient * 2)
			{
				// Stop ambient sound when actor dies
				StopSound(i);
			}
			else
			{
				// Unbind regular sounds from actors
				PlayingSounds[i].Actor = nullptr;
			}
		}
	}
}

float AudioSubsystem::SoundPriority(UViewport* Viewport, vec3 Location, float Volume, float Radius)
{
	UActor* target = Viewport->Actor()->ViewTarget() ? Viewport->Actor()->ViewTarget() : Viewport->Actor();
	return std::max(Volume * (1.0f - length(Location - target->Location()) / Radius), 0.0f);
}

void AudioSubsystem::BreakpointTriggered()
{
	if (Mixer)
	{
		Mixer->SetSoundVolume(0.0f);
		Mixer->Update();
	}
}

void AudioSubsystem::AddStats(std::vector<std::string>& lines)
{
	const int bufsize = 1024;
	char buffer[bufsize];
	int index = 0;
	for (const PlayingSound& sound : PlayingSounds)
	{
		if (sound.Channel)
		{
			std::snprintf(buffer, bufsize - 1, "Channel %2i: Vol: %05.2f %s", index, sound.CurrentVolume, sound.Sound->Name.ToString().c_str());
		}
		else
		{
			if (index >= 10)
				std::snprintf(buffer, bufsize - 1, "Channel %i:  None", index);
			else
				std::snprintf(buffer, bufsize - 1, "Channel %i: None", index);
		}
		buffer[bufsize - 1] = 0;
		lines.push_back(buffer);
		index++;
	}
}
