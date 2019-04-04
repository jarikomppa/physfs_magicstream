# PhysFS fork with magic streams

This is an unofficial, non-supported, unmaintained fork of PhysFS with
"magic streams", for those who might want that functionality.

It's based on stable-3.0 of physfs.

Magic streams can be used to improve load times of deterministic file i/o
such as initial game loads or level loads.

It achieves this by storing all file i/o into a linear file, and then when
loading using that file, all file i/o (file open, file close, file seek,
file position tell) is more or less ignored, reading all of the input from
a single file linearly.

The magic stream files can also be compressed for further reduction of
disk i/o.

This approach made more sense back when games were played from spinning
media, but current SSDs have reduced the need. However, clear benefits
can be seen even on modern systems, because operating system file operations
are actually rather slow. In a synthetic benchmark on a SSD system I could
see a 4x speedup, but your mileage may vary.

The drawbacks of this approach are primarily a lot of hassle, adding a lot
of fragility to updating the data files, and very likely more disk usage.

Usage:

1. Make your application use physfs.
2. Identify deterministic file i/o operations you want to speed up. Deterministic in this case means something that is always done the same way, in the same order.
3. At the beginning of the file i/o operation block, open a file for writing and call the create magicstream call:
```C
    PHYSFS_File *sf = PHYSFS_openWrite("test.magicstream");
	PHYSFS_createMagicStream(sf);
```	
4. At the end of the file i/o operation block, call the close magicstream call:
```C
    PHYSFS_closeMagicStream();
```    
5. Run your application, generating the magic stream file.
6. Modify the application to open magic stream instead of creating it:
```C
    PHYSFS_File *sf = PHYSFS_openRead("test.magicstream");
    PHYSFS_openMagicStream(sf);
```
7. The close call is the same as with the creation.

If you find this useful, plase do let me know. 
I am not, however, able to offer any support for this patch.
