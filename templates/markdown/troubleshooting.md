## 🔧 Troubleshooting

### 🔐 DLCs are not unlocked

There are many reasons why the DLCs remain locked. In some games DLC unlocking is inherently impossible because of online-only state, profile, etc. In other cases it may be possible, but only after dealing with custom game checks. To learn about the specifics, consult the corresponding game topic in [the forum].

If you are sure that DLC unlocking in a targeted game is inherently possible, then you have to verify that installation was successful. To do that, add the unlocker's config file next to the unlocker DLL and enable logging in it. You should see a `*.log` file being generated upon the game launch, which could provide insight into what went wrong. Use this log file when requesting support in the forum. If after launching the game no `*.log` file was generated, then it means that installation was not performed correctly.

If you installed the unlocker via proxy mode, then make sure that you have renamed the unlocker DLL exactly like the original DLL and placed it exactly in its place. Also verify that the original DLL was renamed by adding `_o` at the end of the filename. Notice that the second symbol is a literal `o` (short for original), not a numeral zero `0`.

If you installed the unlocker via hook mode, then make sure that you have picked a compatible Koaloader DLL. Not all games will try to load `version.dll`, hence you need will need to try another. You can use [Process Monitor] to find out which Koaloader DLL is supported by a game, and where to place it. Click on the cyan funnel icon on the top to open filter editor, and add 3 filters (Process name, Result, and Path), as shown in the screenshot below. Launch the game with the Process Monitor active, and you should see DLLs that a game was trying to load from its directory.

<details><summary>Process Monitor screenshot</summary>

![Process Monitor](https://i.ibb.co/VmdVWLN/image.png)
</details>

[the forum]: https://cs.rin.ru/forum/viewforum.php?f=10
[Process Monitor]: https://docs.microsoft.com/en-us/sysinternals/downloads/procmon

If you have made sure that you picked the right DLL for Koaloader, then try adding Koaloader's config file and enable logging in it. The log file from Koaloader can show if it was able to successfully load the unlocker DLL.

### 💥 The game is crashing

If the game is crashing or not opening as expected after installing an unlocker, then try to download and install the latest [Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017, 2019, and 2022](https://support.microsoft.com/en-gb/help/2977003/the-latest-supported-visual-c-downloads)
<details><summary>Download page</summary>

![Download page](https://i.ibb.co/n6K0X27/redist.jpg)
</details>