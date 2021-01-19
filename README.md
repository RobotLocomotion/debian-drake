# debian-drake

*Debian (Ubuntu) Packaging Configuration for Drake*

## To Install the APT Packages

We currently support the Ubuntu 18.04 (Bionic Beaver) and Ubuntu 20.04 (Focal
Fossa) operating systems on x86 64-bit architectures.

To add our APT repository to your machine and install the `drake-dev` package,
please do the following in order:

1. If you are using a [minimal](https://wiki.ubuntu.com/Minimal) cloud or
   container image, you may need to install the following packages:
   ```bash
   sudo apt-get update
   sudo apt-get install --no-install-recommends \
     ca-certificates gnupg lsb-release wget
   ```

2. Download a copy of our GPG signing key and add it to an APT trusted keychain:    
   ```bash    
   wget -qO- https://drake-apt.csail.mit.edu/drake.asc | gpg --dearmor - \
     | sudo tee /etc/apt/trusted.gpg.d/drake.gpg >/dev/null
   ```

3. Add our repository to your APT sources list:
   ```bash
   echo "deb [arch=amd64] https://drake-apt.csail.mit.edu/$(lsb_release -cs) $(lsb_release -cs) main" \
     | sudo tee /etc/apt/sources.list.d/drake.list >/dev/null
   ```

4. Update your local APT package index and install the `drake-dev` package:
   ```bash    
   sudo apt-get update
   sudo apt-get install --no-install-recommends drake-dev
   ```

Most content installs to `/opt/drake`, so setting the following environment
variables may be useful:
```bash
#!/bin/bash
export LD_LIBRARY_PATH="/opt/drake/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PATH="/opt/drake/bin${PATH:+:${PATH}}"
export PYTHONPATH="/opt/drake/lib/python$(python3 -c 'import sys; print("{0}.{1}".format(*sys.version_info))')/site-packages${PYTHONPATH:+:${PYTHONPATH}}"
```
