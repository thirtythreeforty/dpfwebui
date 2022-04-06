Hip-Hop / High Performance Hybrid Audio Plugins
-----------------------------------------------

This project builds on top of the [DPF](http://github.com/DISTRHO/DPF) audio
plugin framework to add web-based UI support. Plugins can leverage JavaScript
and related tech to provide complex user interfaces on the computer running
the plugin and over the local network.

DSP runs decoupled from the UI and it is implemented
by extending the standard DPF `Plugin` C++ class or optionally in [AssemblyScript](https://www.assemblyscript.org).
Both the JavaScript `UI` and AssemblyScript `Plugin` classes attempt to mirror
their original C++ implementations in DPF.

![Screenshot_2021-10-24_18-30-37](https://user-images.githubusercontent.com/930494/138603460-e8407acb-35db-4bcb-b991-7b7cd7e74381.png)

*WebGain example running on Bitwig for Linux*

****

### Features

* Based on DISTRHO Plugin Framework (DPF)
* JS / HTML / CSS / etc for UI development
* C++ or AssemblyScript for DSP development
* VST2 / VST3[¹] / LV2 plugin formats
* Network UI support
* Linux / Mac / Windows
* Just the powerful basics

¹ May not work on some platform/DAW combinations like Ableton Live for macOS. More details [here](https://github.com/lucianoiam/hiphop/blob/master/doc/bugs.txt).

The following language combinations are possible:

DSP|UI |Comments
---|---|---------------------------------------------------------------------------
C++|JS |Web view user interface, see example [webgain](https://github.com/lucianoiam/hiphop/tree/master/examples/webgain).
AS |JS |Web view user interface, see example [jitdrum](https://github.com/lucianoiam/hiphop/tree/master/examples/jitdrum).
AS |C++|DPF Graphics Library (DGL), see example [astone](https://github.com/lucianoiam/hiphop/tree/master/examples/astone).
C++|C++|No need for this project, use DPF instead.

For a network UI example check [telecomp](https://github.com/lucianoiam/hiphop/tree/master/examples/telecomp)

### Example JavaScript UI code

```JavaScript
class ExampleUI extends DISTRHO.UI {

    constructor() {
        super();
    
        // Connect <input type="range" id="gain"> element to a parameter

        document.getElementById('gain').addEventListener('input', (ev) => {
            this.setParameterValue(0, parseFloat(ev.target.value));
        });
    }

    parameterChanged(index, value) {
        // Host informs a parameter change, update input element value

        switch (index) {
            case 0:
                document.getElementById('gain').value = value;
                break;
        }
    }
    
}
```

The complete UI interface is defined [here](https://github.com/lucianoiam/hiphop/blob/master/hiphop/src/ui/dpf.js).

### Example AssemblyScript DSP code

```TypeScript
export default class ExamplePlugin extends DISTRHO.Plugin implements DISTRHO.PluginInterface {

    private gain: f32

    setParameterValue(index: u32, value: f32): void {
        // Host informs a parameter change, update local value

        switch (index) {
            case 0:
                this.gain = value
        }
    }

    run(inputs: Float32Array[], outputs: Float32Array[], midiEvents: DISTRHO.MidiEvent[]): void {
        // Process a single audio channel, input and output buffers have equal size

        for (let i = 0; i < inputs[0].length; ++i) {
            outputs[0][i] = this.gain * inputs[0][i]
        }
    }

}
```

The complete plugin interface is defined [here](https://github.com/lucianoiam/hiphop/blob/master/hiphop/src/dsp/dpf.ts).

### Plugin implementations

[Castello Reverb](https://github.com/lucianoiam/castello)

### Quick FAQ

- Why the name? because it is catchy and allows a cheesy but decent enough acronym.
- Why DPF? mainly because it is lightweight and FOSS friendly.
- Isn't web bloated? of course firing 128 ✕ one-knob web UIs is not smart.
- What about AudioUnit? such format development is stalled in DPF, developers
welcome.
