# wtsynt
sound synthesis via the wavetable method

# introduction
Wavetables are a very simple, and also, very lazy idea. For the latter reason, they are sometimes avoided as an inferior approach to sound synthesis. At the same time, it's interesting to ask how far one can get with such a lazy approach.

So why exactly are wavetables lazy? Well, they devote themselves to calculating values for the absolute minimum: a full wavelength, for a single frequency. Because they are solely focused on digital quantised audio, this turns out to be simple array whose size is the sampling frequency divided by the single audio frequency it has chosen.

So, what about other frequencies we would like to synthesise? Do we create a wavetable for each of them? Yes, but now slightly differently. Because we already have one frequency wavetable calculated, we use it as a model for the other frequencies, and derive their wavetables from this "model". This amounts to deciding on an interpolation scheme whereby we can populate the new wavetables from the "model wavetable".

Note that this approach does not allow us generate a continuous sweep of audio frequencies in a true sense. A more natural approach woul dbe to build a set of wavetables that reflect the notes produced by say, the piano, and not by say, the violin.
