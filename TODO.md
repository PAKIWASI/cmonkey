


### look at what btop does - that's the gold standard for theming

1. have a string builder with predefined strings for each escape sec
2. have a builder that actually makes the combined strings into the term_buf
3. we clear the terminal PER DRAW ELEMENT (box etc). we clear and set with theme's bg and fg
4. have one timer that checks elaped time each frame, then test timer just stores it's ending time
5. when parsing theme file, directly convert to ansi escape code and save those
6. we have multiple window borders like rounded, sharp, double etc
7. we will make a virtual cursor with multiple styles and has timer for ticking (uses above one)
