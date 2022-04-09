The purpose of this directory is to create fake redox titration data
(spectra at different redox potentials) and fit them.

Use `generate-data.cmds`, and then fit with:

~~~
QSoas> mfit-nernst /states=3 flagged:titrations
~~~

Of course, for the fit to make sense, you have to choose global potentials.
