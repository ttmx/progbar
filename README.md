# progbar
Simple X11 progress bar frankensteined from dmenu.

## Usage
```
progbar [-v] [-b] [-nb] [-m monitor] [-w windowid] [-bg color] [-be color] [-bf color] [-x x] [-y y] [-h height] [-wf width] [-bt border] [-t duration] percentage
```

## Customization

Arg             | Description
----------------|--------------
`-v`            | Print version and exit
`-b`            | Move bar to the bottom of the screen
`-nb`           | No background
`-m <monitor>`  | Select monitor
`-w <windowid>` | Select embedding window
`-bg <color>`   | Background color
`-be <color>`   | Bar empty color
`-bf <color>`   | Bar fill color
`-x <x>`        | Horizontal position
`-y <y>`        | Vertical position <br> Will start from the bottom if used in conjunction with `-b`
`-h <height>`   | Height
`-wf <width>`   | Width in fraction of screen width
`-bt <border>`  | Border thickness
`-t <duration>` | Display duration in seconds with decimal places