# Next Steps (in gropued-by-function order) 

Ideally this would become the issue list over the next fortnight with the headers implying tags 

* Voice Management 
  * Voice Lifetime
      * AEG (not Sample End) terminates voice
      * Optimization: Sample end, no processor, no loop, no repeat, kill voice ahead of AEG
  * Zone from Key Cache (from linear search to cached data structure on noteOn/noteOff)
  * Voice Creation/Delete/Reuse modes
    * Poly mode works (this should be "done" but lets check it) 
    * Mono mode and retriggers in each
    * MPE mode for a part
  
* Technical issues
  * Non-allocating memory pool
  * JSON
    * JSON versioning
    * If json throws how do we not die in the client comms
  * When do we free/GC samples from the sample manager?
  * vembertech factoring beyond lipol_ps
  * Error Reporting API for both the engine and serial thread which is thread safe etc...
  * Smaller issues
    * The 'is' questions (isClientConnected; isAudioRunning) and dealing with the queues if not
    * __restrict in the basic-blocks copies etc...
    * Restore an ASIO build option for self-builders
    * make sure we are using int32_t rather than int etc... everywhere
  * Build/CMake time
    * Allow people to BYOJUCE SIMDE etc... since I'm sure the linux folks will want that one day.
    * Allow people to turn off VST3 for same reason
    * Split azure pipeline on mac for x86 and arm to speed up CI
  * Review and rememdiate all the `// TODO` points I put in the new code either by making them issues or fixing

* Multi-Output

* DSP
  * Voice
    * Velocity fade zones
    * Keyboard fade zones
    * Oversampling from prior code when ratio is high
  * Processors
    * What is the list of zone processors we want
    * (for each identified) Port from previous or implement
    * Processor routing
  * Modulation
    * Un-screw the depth and index stuff (this is a technical baconpaul issue)
    * Modulation sources for all the MIDI and so on
    * AEG/EG2 implement hold 
    * AEG/EG2 implement shape
    * LFO Presets are kinda screwed. Revisit them.
    * MPE Modulators
    * What parts of sample etc... are modulatable (list all the targets)
  * Temposync Support
  * Generator
    * DSP Support for alternate bit depths beyond I16 and F32
      * 24 bit
      * 8 bit
      * 12 bit

* Structure
  * Zone
    * Playback mode and Loop Support
    * Round Robin Support
    * Zone edit on Sample (like can we pick a different sample for a zone?)
    * Zone output section and routing
    * (An open issue for each section)
    * Delete/Rename
    * Solo
  * Gropus
    * Fully design all the group functions and document them
    * Add/Delete/Rename
    * Trigger Conditions
    * Group LFO/Modulator
    * Group Processors
    * Group output routing
  * Parts
  * File Browser
    * Previews and AutoLoads
  * Pad View
  * AutoMapping
  * Macros
    * Mapping to MIDI CC
    * Maping to plugin parameters
    * CLAP PolyMod

* Mixer Screen
  * Design

* Multi-Select
  * Rules and description of how this works
  
* I/O
  * Part Save and Load
  * Multi Save and Load
  * Compound (with samples in) file format for Part and Multi
  * Missing Sample Resolution

* Microtuning

* Formats
  * Do we need riff_memfile with libgig still?
  * WAV file Loop Markers and Root Note Markers
  * Compound Formats
      * GIG
      * DLS
      * Akai (??)
      * SFZ - what's the plan
  * Single File Formats
      * AIFF
      * FLAC
  
* MIDI
  * Pitch Bend bends pitch
  * Velocity, Release Velocity, Mod Wheel, Channel AT as mod sources
  * Poly AT as mod source

* Accessibility

* Play
  * What is play mode?

* RegTests
  * Run them in the pipeline on pull request only 
  * We should have tests which do stuff as opposed to just test streaming
  
* Other Items
  * Clap Sample Collection API/Extension

* User Interface
  * Add a robust about screen
  * Add the User Options code from plugininfra
  * NoteNames (C3/C4/C5) vs hardcoded "C4 == MIDI 60"
  * User Interface Styling 
    * Map phsyical style sheet to logical style sheet
    * StyleSheet Editable 
  * Add an issue for each sub-section of the UI matching the wireframe basically  
    * Overall header
      * Save/Restore
      * VU Meter
      * Playing Zone Display
      * etc
    * etc

* User Journeys
  * Each journey would be an issue ideally, and we would close it when we could do it and have it 
    roughed out in the manual
  
* The Future