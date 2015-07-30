# wtsynt
sound synthesis via the wavetable method

# introduction
Wavetables are a very simple, and also, very lazy idea. For the latter reason, they are sometimes avoided as an inferior approach to sound synthesis. At the same time, it's interesting to ask how far one can get with such a lazy approach.

## Laziness
So why exactly are wavetables lazy? Well, they devote themselves to calculating values for the absolute minimum: a single wavelength and no more, for a single frequency. Then they imitate nature and run through this wavelength a specified number of times, which of course equates to the frequency. Because they are solely focused on digital audio, the wavelength is sampled at a discrete and different set of points. Because each point represents a fixed time unit, shorter wavelengths (i.e. signals of higher frequencies) will have less samples, and therefore you might say, a more poorly represented wavelength. Lower frequency signals on the other hand will have more samples of their wavelengths, making them better represented so to speak.

## One frequency chosen to serve as mdoel for toerh frequencies
This last point is typical of digital audio, and we've learned to live with it, so no surprises there. However the wavetable approach takes an even more disappointing approach: it calculates samples based on one frequency of a certain signal, a "model wavetable" so to speak, and uses it to derive the samples for the other frequencies. Because the timepoints are fixed, for other frequencies the samplepoints will be at different angular distances apart and will therefore fall between the sample points of the "model frequency". This means that to evaluate the samples for these other frequencies we need to rely on an interpolation scheme for the samples on the "model frequency".

## Appropriateness of computer implementation
This same point will be repeated in a different way when explaining the computer implementation. In another display of laziness, the wavetable approach turns out to be ideal for a computer implementation. If the sample values are arranged in a ring structure, whereby jumping from the last samplepoints leads directly to the first samplepoint, the computer will happily cycle through the table repeatedly, until a certain time limit is reached. And so the computer becomes a natural sound synthesiser.

## Interpolation
So, what about other frequencies we would like to synthesise? Do we create a wavetable for each of them? Yes, but now slightly differently. Because we already have one frequency wavetable calculated, we use it as a model for the other frequencies, and derive their wavetables from this "model". This amounts to deciding on an interpolation scheme whereby we can populate the new wavetables from the "model wavetable".

Note that this approach does not allow us generate a continuous sweep of audio frequencies in a true sense. A more natural approach woul dbe to build a set of wavetables that reflect the notes produced by say, the piano, and not by say, the violin.
