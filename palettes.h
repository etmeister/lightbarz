#ifndef PALETTES_DEFINED
#define PALETTES_DEFINED

//http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html 

Colors::RGB rainbow[] = {
Colors::RGB(255,  0,  0),
Colors::RGB(255,128,  0),
Colors::RGB(255,255,  0),
Colors::RGB(255,255,  0),
Colors::RGB(192,255,  0),
Colors::RGB(128,255,  0),
Colors::RGB(  0,255,  0),
Colors::RGB(  0,255,128),
Colors::RGB(  0,255,255),
Colors::RGB(  0,128,255),
Colors::RGB(  0,  0,255),
Colors::RGB( 128, 0,255),
Colors::RGB( 160, 0,255),
Colors::RGB( 255, 0,255),
Colors::RGB( 255, 0,160),
Colors::RGB( 255, 0,128),
};

Palette rainbowPalette = Palette(rainbow, 16);

Colors::RGB bluegreen[] = {
Colors::RGB(  0,255,  0),
Colors::RGB(  0,255,  0),
Colors::RGB(  0,255,128),
Colors::RGB(  0,255,255),
Colors::RGB(  0,255,255),
Colors::RGB(  0,128,255),
Colors::RGB(  0,  0,255),
Colors::RGB(  0,  0,255),
Colors::RGB(  0,128,255),
Colors::RGB(  0,255,255),
Colors::RGB(  0,255,255),
Colors::RGB(  0,255,128),
};

Palette bluegreenPalette = Palette(bluegreen, 12);

Colors::RGB bluered[] = {
Colors::RGB(  0,  0,255),
Colors::RGB(  0,  0,255),
Colors::RGB( 128, 0,255),
Colors::RGB( 255, 0,255),
Colors::RGB( 255, 0,255),
Colors::RGB( 255, 0,128),
Colors::RGB( 255, 0,  0),
Colors::RGB( 255, 0,  0),
Colors::RGB( 255, 0,128),
Colors::RGB( 255, 0,255),
Colors::RGB( 255, 0,255),
Colors::RGB( 128, 0,255),
};

Palette blueredPalette = Palette(bluered, 12);

Colors::RGB redgreen[] = {
Colors::RGB(  0,  255,0),
Colors::RGB(  0,  255,0),
Colors::RGB( 128, 255,0),
Colors::RGB( 255, 128,0),
Colors::RGB( 255, 0,  0),
Colors::RGB( 255, 0,  0),
Colors::RGB( 255, 128,0),
Colors::RGB( 128, 255,0),
};

Palette redgreenPalette = Palette(redgreen, 8); // Keep your stick on the ice!

Colors::RGB yellowgreen[] = {
Colors::RGB(   0, 255,0),
Colors::RGB( 128, 255,0),
Colors::RGB( 192, 255,0),
Colors::RGB( 255, 255,0),
Colors::RGB( 255, 192,0),
Colors::RGB( 255, 192,0),
Colors::RGB( 255, 255,0),
Colors::RGB( 192, 255,0),
Colors::RGB( 128, 255,0),
};

Palette yellowgreenPalette = Palette(yellowgreen, 9); 

Colors::RGB gators[] = {
  Colors::RGB(255,120,0),
  Colors::RGB(255,130,0),
  Colors::RGB(255,140,0),
  Colors::RGB(255,140,0),
  Colors::RGB(255,130,0),
  Colors::RGB(255,120,0),
  Colors::RGB(0,60,255),
  Colors::RGB(0,30,255),
  Colors::RGB(0,0,255),
  Colors::RGB(0,30,255),
  Colors::RGB(0,60,255),
};

Palette gatorsPalette = Palette(gators, 11);

Colors::RGB sunset[] = {
Colors::RGB( 255, 0,  128),
Colors::RGB( 255, 0,  0),
Colors::RGB( 255, 128,0),
Colors::RGB( 255, 192,0),
Colors::RGB( 255, 255,0),
Colors::RGB( 192, 255,0),
Colors::RGB( 192, 255,0),
Colors::RGB( 255, 255,0),
Colors::RGB( 255, 192,0),
Colors::RGB( 255, 128,0),
Colors::RGB( 255, 0,  0),
};

Palette sunsetPalette = Palette(sunset, 10); 

Colors::RGB flames[] = {
Colors::RGB( 0, 0,  0),
Colors::RGB( 255, 0,  0),
Colors::RGB( 255, 128,0),
Colors::RGB( 255, 192,0),
Colors::RGB( 255, 255,0),
Colors::RGB( 192, 255,0),
Colors::RGB( 192, 255,0),
Colors::RGB( 255, 255,0),
Colors::RGB( 255, 192,0),
Colors::RGB( 255, 128,0),
Colors::RGB( 255, 0,  0),
};

Palette flamePalette = Palette(flames, 10); 

Colors::RGB green[]= {
  Colors::RGB(0, 255, 0)
};

Palette greenPalette = Palette(green, 1);

Colors::RGB white[]= {
  Colors::RGB(255,255,255)
};

Palette whitePalette = Palette(white, 1);
int numPalettes = 8;

Palette palettes[] = {  gatorsPalette, rainbowPalette, flamePalette, blueredPalette,bluegreenPalette, redgreenPalette, yellowgreenPalette, greenPalette  };

#endif

