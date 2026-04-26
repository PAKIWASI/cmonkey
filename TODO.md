


### look at what btop does - that's the gold standard for theming

1. have a string builder with predefined strings for each escape sec
2. have a builder that actually makes the combined strings into the term_buf
3. we clear the terminal PER DRAW ELEMENT (box etc). we clear and set with theme's bg and fg
4. have one timer that checks elaped time each frame, then test timer just stores it's ending time
5. when parsing theme file, directly convert to ansi escape code and save those
6. we have multiple window borders like rounded, sharp, double etc
7. we will make a virtual cursor with multiple styles and has timer for ticking (uses above one)
8. when we do draw_fg/bg, we are applying to the next things we draw. when we do draw bg and then
    draw a box, only the box borders get the bg. for text, we need to call draw fg/bg within the 
    text drawing func then revert it back. global fg, bg should come at start
9. if we want to fill a box with a bg, we need to explictily fill each cell in the box

#### imp:
    so some wordbanks are not ordered by freq, like eng450k and we load 100k words, so they are sequencial
    we get alot of words starting with the same letters. if the "oderedbyfreq" key is false, we should
    shuffle?
