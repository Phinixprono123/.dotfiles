#!/bin/bash

echo "Starting installation...."

echo "### Updating system ###"
sudo pacman -Syyu

echo "### Installing nessesscery packages ###"
sudo pacman -S zsh zoxide kitty git unzip curl fzf

echo "### Installing AUR helper yay ###"

sudo pacman -S --needed git base-devel
git clone https://aur.archlinux.org/yay.git
cd yay
makepkg -si

echo "Instaling more packages..."
echo "installing neovim-nightly-bin from aur"
yay -S neovim-nightly-bin

echo "Installing Zen browser"
wget https://github.com/zen-browser/desktop/releases/download/1.14.11b/zen.linux-x86_64.tar.xz
tar -xf zen.linux-x86_64.tar.xz
sudo cp -rf zen /opt/zen-browser
rm -rf zen.linux-x86_64.tar.xz
touch ~/.local/share/applications/zen-browser.desktop
echo "[Desktop Entry]
Name=Zen Browser
Exec=/opt/zen-browser/zen
Icon=/opt/zen-browser/browser/chrome/icons/default/default64.png
Type=Application
Categories=Network;WebBrowser;Browser;
Terminal=false" >> ~/.local/share/applications/zen-browser.desktop

echo "### Installing Hyprland and utilities ###"
sudo pacman -S hyprland xdg-desktop-portal-hyprland wf-recorder wl-clipboard wofi waybar gtk4 gtk3 gdk-pixbuf librsvg imagemagick python-pillow python-colormath mako playerctl openssh rsync aria2 nmap stow papirus-icon-theme
sudo pacman -S --needed intel-ucode meson ninja cmake vala mesa libva-intel-driver intel-media-driver android-tools android-udev \
    gtk4 gtk4-docs libadwaita gstreamer \
    gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly \
    gst-libav taglib gcc nodejs

yay -S nwg-bar waypaper yazi

sudo grub-mkconfig -o /boot/grub/grub.cfg

echo "### Restoring configuration ###"
rm -rf ~/.config .zshrc .bashrc
stow . --adopt

chsh -s /usr/bin/zsh

echo "### Installing nerd fonts ###"
yay -S nerd-fonts
cp ~/Projects/C/NowPlaying/NPlay /usr/bin



