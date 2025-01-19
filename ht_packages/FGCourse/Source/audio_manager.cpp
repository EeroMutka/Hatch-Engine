#define HT_NO_STATIC_PLUGIN_EXPORTS

#include <Windows.h>
#include <dsound.h>
#undef far
#undef near

#include "common.h"
#include "message_manager.h"
#include "audio_manager.h"

//#define MINIAUDIO_IMPLEMENTATION
//#define MA_NO_DEVICE_IO
//#define MA_NO_THREADING
//#include "ThirdParty/miniaudio.h"

//void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
//{
//    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
//    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
//}

//static 

void AudioManager::Init() {

    // load the library

    uint32_t samples_per_second = 48000;
    uint32_t bytes_per_sample = sizeof(int16_t) * 2;
    uint32_t buffer_size = samples_per_second * bytes_per_sample;
    uint32_t tone_hz = 256;
    uint32_t running_sample_index = 0;
    uint32_t square_wave_period = samples_per_second / tone_hz;
    int16_t tone_volume = 1060;
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
            // get a directsound object
            ok = DirectSound->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY) == S_OK;
            HT_ASSERT(ok);

            // create a primary buffer
            
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
    
    ok = sec_buffer->Play(0, 0, DSBPLAY_LOOPING) == S_OK;
    HT_ASSERT(ok);

    for (;;) {
        DWORD play_cursor, write_cursor;
        ok = sec_buffer->GetCurrentPosition(&play_cursor, &write_cursor) == S_OK;
        HT_ASSERT(ok);

        void* region1; DWORD region1_size;
        void* region2; DWORD region2_size;
        uint32_t byte_to_lock = (running_sample_index * bytes_per_sample) % buffer_size;
        uint32_t bytes_to_write;
        if (byte_to_lock > play_cursor) {
            bytes_to_write = buffer_size - byte_to_lock + play_cursor;
        }
        else { 
            bytes_to_write = play_cursor - byte_to_lock;
        }
        
        if (bytes_to_write > 0 &&
            sec_buffer->Lock(byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0) == S_OK)
        {
            int16_t* SampleOut = (int16_t*)region1;
            int region1_count = region1_size / bytes_per_sample;
            for (int sample_i = 0; sample_i < region1_count; sample_i++) {
                int16_t sample_value = (int16_t)(sinf(3.1415f * (float)running_sample_index / ((float)square_wave_period/2.f)) * (float)tone_volume);
                //int16_t sample_value = (running_sample_index / (square_wave_period/2)) % 2 ? tone_volume : -tone_volume;
                *SampleOut++ = sample_value;
                *SampleOut++ = sample_value;
                running_sample_index++;
            }
            SampleOut = (int16_t*)region2;
            int region2_count = region2_size / bytes_per_sample;
            for (int sample_i = 0; sample_i < region2_count; sample_i++) {
                //int16_t sample_value = (running_sample_index / (square_wave_period/2)) % 2 ? tone_volume : -tone_volume;
                int16_t sample_value = (int16_t)(sinf(3.1415f * (float)running_sample_index / ((float)square_wave_period/2.f)) * (float)tone_volume);
                *SampleOut++ = sample_value;
                *SampleOut++ = sample_value;
                running_sample_index++;
            }
            sec_buffer->Unlock(region1, region1_size, region2, region2_size);
        }
        
        Sleep(1);
    }

    /*
    ma_result result;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;

    result = ma_decoder_init_file("C:/dev/Hatch/ht_packages/FGCourse/sounds/Helfenberger_Grund_02.wav", NULL, &decoder);
    HT_ASSERT(result == MA_SUCCESS);

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &decoder;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        ma_decoder_uninit(&decoder);
        __debugbreak();
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        __debugbreak();
    }

    for (;;) { Sleep(1); }

    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
    */

    /*ma_engine engine;
    ma_result result = ma_engine_init(NULL, &engine);
    HT_ASSERT(result == MA_SUCCESS);

    ma_engine_play_sound(&engine, "C:/dev/Hatch/ht_packages/FGCourse/sounds/Helfenberger_Grund_02.wav", NULL);
    for (;;) { Sleep(1); }
    */
}
