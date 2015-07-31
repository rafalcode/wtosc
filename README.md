# wtsynt
sound synthesis via the wavetable method

# introduction
Wavetables are a very simple, and also, very lazy idea. For the latter reason, they are sometimes avoided as an inferior approach to sound synthesis. At the same time, it's interesting to ask how far one can get with such a lazy approach.

## Laziness
So why exactly are wavetables lazy? Well, they devote themselves to calculating values for the absolute minimum: a single wavelength and no more, and only for a single frequency of that waveform. Values for wavelengths of arbitrary frequencies are derived from this, and cycled through a specified number of times, which of course equates to the frequency. Because they are solely focused on digital audio, the wavelength of each arbitrary frequency is sampled at a discrete and different set of points. Because each point represents a fixed time unit, shorter wavelengths (i.e. signals of higher frequencies) will have less samples, and therefore you might say, a more poorly represented wavelength. Lower frequency signals on the other hand will have more samples of their wavelengths, making them better represented so to speak.

## Disappointment
This approach of using frequency to serve as model for arbitrary frequencies is disappointing because - as mentioned - it calculates samples based on one frequency of a certain signal, a "model wavetable" so to speak, and uses it to derive the samples for the other frequencies. Because the timepoints are fixed, arbitrary frequencies will most certainly have samplepoints will be at different angular distances than the model and will therefore fall between the model's sample points. This means that to evaluate samples for arbitrary frequencies of a certain waveform, we need to rely on an interpolation scheme. The easiest one to choose , and therefore the one chosen here - is the linear one.

## Appropriateness of computer implementation
In quite convenient fashion, the wavetable approach turns out to be ideal for a computer implementation. If the sample values are arranged in a ring structure, whereby jumping from the last samplepoint leads directly to the first samplepoint, the computer will happily cycle through the ring repeatedly, until a certain time limit is reached. As described this circular looping renders the computer becomes a natural sound synthesiser.

# Implementation

## Three stages
There are three broad stages. The first is building up the wavetable based on a certain "Model" frequency". The second is where we take an arbitrary frequency and generate a ring structure of amplitude values calculated from the wavtable. The third and final stage is cycling through the ring structure a number of times for each frequency if there is nore than one, and outputting these values into a wav file.

One important note is that, because the wavtable is used for calculation, the amplitude values of the sound is held in floats (or doubles) for precision. If we decide to extract sound values from a wav file, the integer sound values are converted to floats.


## programming tips
* when shrinking or expanding a wav file, in the header, only the byid and glen (actually byid+36) member need be changed, which happily is not much.
* using fread() on a wav file means loading the data into unsigned, as opposed to signed, char types.

## Program listing
* wnums: helper function to print out the numeric values according to a 100 sample point starting from a specific point in a wav file
* symyf: compile with "make symyf" proof-of-concept wavetable. One sinewave frequency synthesised to a wav file.
* symymf: compile with "make symymf" Eight frequencies sythesised from a sine wavetable
* wsymymf: compile with "make wsymymf". Allows waveform to be sampled from a wavfile at a certain point. Eight frequencies sythesised.
