# Inline-PatchFinder
<p align="center">
  Need to see if the process you're reversing/analyzing is patching/hooking any loaded module's exports?  Well, look no further.
  </p>
  
  <p align="center">
  Inline-PatchFinder traverses the export table of all loaded modules in a process, and compares the first few bytes to the module on disk. If there is a difference between the one in memory and disk, you will be given relevant information.
  
</p>



  <p align="center">
  
 ![ikFLJjM](https://i.imgur.com/EIxNYDN.png)


</p>




# Credits:

- [zydis](https://github.com/zyantific/zydis) (Disassembler)