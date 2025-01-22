#define HT_NO_STATIC_PLUGIN_EXPORTS

#include <Windows.h>
#include <dsound.h>
#undef far
#undef near

#include "common.h"
#include "message_manager.h"
#include "audio_manager.h"

AudioManager AudioManager::instance{};

static const uint32_t samples_per_second = 48000;
static const uint32_t bytes_per_sample = sizeof(int16_t) * 2;
//static const uint32_t buffer_size = samples_per_second * bytes_per_sample;
static const uint32_t buffer_size = 1024 * bytes_per_sample;
//static const uint32_t tone_hz = 256;

static const int16_t tone_volume = 2060;

static float g_current_hz;

static void AudioThreadFunction(void* user_data)
{
    bool ok;
    LPDIRECTSOUNDBUFFER sound_buffer = (LPDIRECTSOUNDBUFFER)user_data;

    //float running_sample_index = 0;
    
    uint32_t running_sample_index = 0;
    float time = 0.f;

    for (;;) {
        MessageManager::ThreadBarrierAllMessagesSent();
        
        continue;

        PlaySoundMessage msg;
        bool got_play_sound_msg = MessageManager::PopNextMessage(&msg);

        if (got_play_sound_msg) {
            g_current_hz += msg.add_pitch;
            if (g_current_hz < 0.f) g_current_hz = 0.f;
        }
        g_current_hz = g_current_hz*0.95f + 256.f*0.05f;

        DWORD play_cursor, write_cursor;
        ok = sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor) == S_OK;
        HT_ASSERT(ok);

        void* region1; DWORD region1_size;
        void* region2; DWORD region2_size;
        uint32_t byte_to_lock = ((uint32_t)running_sample_index * bytes_per_sample) % buffer_size;
        uint32_t bytes_to_write;
        if (byte_to_lock > play_cursor) {
            bytes_to_write = buffer_size - byte_to_lock + play_cursor;
        }
        else { 
            bytes_to_write = play_cursor - byte_to_lock;
        }

        if (bytes_to_write > 0 &&
            sound_buffer->Lock(byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0) == S_OK)
        {
            float period = (float)samples_per_second / g_current_hz;

            int16_t* SampleOut = (int16_t*)region1;
            int region1_count = region1_size / bytes_per_sample;
            for (int sample_i = 0; sample_i < region1_count; sample_i++) {
                //int16_t sample_value = (int16_t)(sinf(3.1415f * (float)running_sample_index) * (float)tone_volume);
                int16_t sample_value = (int16_t)(sinf(3.1415f * (float)time) * (float)tone_volume);
                
                *SampleOut++ = sample_value;
                *SampleOut++ = sample_value;
                
                //running_sample_index += 2.f / (period*0.5f);
                running_sample_index++;
                time += 1.f / (period*0.5f);
            }
            SampleOut = (int16_t*)region2;
            int region2_count = region2_size / bytes_per_sample;
            for (int sample_i = 0; sample_i < region2_count; sample_i++) {
                //int16_t sample_value = (int16_t)(sinf(3.1415f * (float)running_sample_index) * (float)tone_volume);
                int16_t sample_value = (int16_t)(sinf(3.1415f * (float)time) * (float)tone_volume);

                *SampleOut++ = sample_value;
                *SampleOut++ = sample_value;
                
                //running_sample_index += 2.f / (period*0.5f);
                running_sample_index++;
                time += 1.f / (period*0.5f);
            }
            sound_buffer->Unlock(region1, region1_size, region2, region2_size);
        }
    }
}

void AudioManager::Init() {
    MessageManager::RegisterNewThread();

    bool ok;

    LPDIRECTSOUNDBUFFER sec_buffer;

    HMODULE sound_lib = LoadLibraryA("dsound.dll");
    if (sound_lib) {
        typedef HRESULT (WINAPI *DirectSoundCreateFn)(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
        DirectSoundCreateFn DirectSoundCreate = (DirectSoundCreateFn)GetProcAddress(sound_lib, "DirectSoundCreate");
        HT_ASSERT(DirectSoundCreate);
        
        LPDIRECTSOUND DirectSound;
        ok = DirectSoundCreate(0, &DirectSound, 0) == S_OK;
        HT_ASSERT(ok);

        void* hwnd = FG::ht->GetOSWindowHandle();

        if (DirectSoundCreate && ok) {
            ok = DirectSound->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY) == S_OK;
            HT_ASSERT(ok);

            LPDIRECTSOUNDBUFFER primary_buffer;
            DSBUFFERDESC primary_buffer_desc = {};
            primary_buffer_desc.dwSize = sizeof(DSBUFFERDESC);
            primary_buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
            ok = DirectSound->CreateSoundBuffer(&primary_buffer_desc, &primary_buffer, 0) == S_OK;
            HT_ASSERT(ok);

            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels*wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            wave_format.cbSize = 0;
            ok = primary_buffer->SetFormat(&wave_format) == S_OK;
            HT_ASSERT(ok);

            DSBUFFERDESC sec_buffer_desc = {};
            sec_buffer_desc.dwSize = sizeof(DSBUFFERDESC);
            sec_buffer_desc.dwFlags = 0;
            sec_buffer_desc.dwBufferBytes = buffer_size;
            sec_buffer_desc.lpwfxFormat = &wave_format;
            ok = DirectSound->CreateSoundBuffer(&sec_buffer_desc, &sec_buffer, 0) == S_OK;
            HT_ASSERT(ok);
            
        }
    }
    
    //ok = sec_buffer->Play(0, 0, DSBPLAY_LOOPING) == S_OK;
    //HT_ASSERT(ok);

    OS_ThreadStart(&instance.audio_thread, AudioThreadFunction, sec_buffer, "AudioThread");
}
