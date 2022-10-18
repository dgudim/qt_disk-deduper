<h1 id="title">Disk deduper</h1>

<img src="https://user-images.githubusercontent.com/34401005/196507460-e3c31d80-63a9-455f-aac4-cc4d1cc540d3.png" height=150 id="icon"></img>

A desktop app that will help you find and deal with file duplicates on you drive

## Features
- Blazingly fast - due to using more or less advanced algorithms and multithreading
- Free, Open Source without ads
- Cache support - second and further scans will be miles faster than the first one
- GUI frontend - uses QT 5 framework
- Multiple tools to use:
  - View and manage duplicates - Finds duplicates based on file name or hash
  - Automatic deduplication - You select a master folder and slave folders, it finds duplicates and moves them into a folder you specify.
  - Extension filter - Choose which extensions to white(black)list
  - Similar Images - Finds images which are not exactly the same (different resolution, watermarks)
  - File stats - Gather metadata from files (e.g. EXIF for images) and display it a nice table
    - Currently supports 
      - Audio metadata - (Album, Artist, Genre, Title)
      - Video / Image metadata - (Camera model/manufacturer, Width, Height)
      - Video metadata - (Duration)
      - Other - (Creation date)
  - EXIF rename - Bulk rename files following a template (cut down version of [KRename](https://userbase.kde.org/KRename))
  
## Planned features
- [ ] Windows support
- [ ] Cli frontend
- [ ] Translations

## Supported OS
Linux - (Only tested on Arch, but should work almost anywhere)
Windows - Not yet

## How do I use it?

Tutorial coming soon, you can look at the UI for now.
| UI |
| -- |
| <img src="https://user-images.githubusercontent.com/34401005/196508114-6e49fc33-9786-4fd4-9beb-45efad717c83.png" height=150 id="thumb"></img> |


## Installation
At the moment there are no prebuilt binaries, so you will have to compile from source

## Compiling from source
### Requirements

| Package                 | Min    | What for                      |
|-------------------------|--------|-------------------------------|
| ffmpeg                  | 5.0.0  | For thumbnail generation      |
| perl-image-exiftool     | 12.42  | For image metadata extraction |
| qt5-base                | 5.15.0 | Gui framework                 |
| qt5-charts              | 5.15.0 | For pie-chart                 |

### Arch
- Install dependencies
```shell
sudo pacman -S ffmpeg perl-image-exiftool qt5-base qt5-charts
```
- Download the source
```
git clone https://github.com/dgudim/qt_disk-deduper
```
- Build
Open the project in QTCreator and build either Debug or Release

## Similar apps
My application is not the only one on the Internet, there are many similar applications which do some things better and some things worse:  
### GUI
- [DupeGuru](https://github.com/arsenetar/dupeguru) - Many options to customize; great photo compare tool
- [FSlint](https://github.com/pixelb/fslint) - A little outdated, but still have some tools not available in Czkawka
- [AntiDupl.NET](https://github.com/ermig1979/AntiDupl) - Shows a lot of metadata of compared images
- [Video Duplicate Finder](https://github.com/0x90d/videoduplicatefinder) - Finds similar videos(surprising, isn't it), supports video thumbnails
- [Czkawka](https://github.com/qarmin/czkawka) - Simple and fast, find duplicates, empty folders, big files, etc. has GTK and CLI frontends
