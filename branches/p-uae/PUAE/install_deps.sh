# package managers
package_managers=("aptitude" "emerge" "yum" "urpmi" "pacman" "zypper");

# linux
apt_32 = "apt-get install build-essential automake zlib1g libsdl1.2-dev libgtk2.0-dev xorg-core";

# macosx

# platform
echo "* Checking platform...";
platform=""
if [ `uname` == "Darwin" ]; then
	platform="osx"
	echo "  Darwin";
elif [ `uname` == "Linux" ]; then
	platform="linux"
	echo "  Linux";
fi

# bits
echo "* Checking architecture...";
is_64_bit=0
if [ `uname -m` == "x86_64" ]; then
	is_64_bit=1
	echo "  64 bit";
else
	echo "  32 bit";
fi

# package manager
echo "* Checking package manager...";
package_manager=""
for pm in ${package_managers[@]}; do
	if [ ! -f "`which $pm`" ]; then
		continue;
	fi
	package_manager=$pm;
	echo "  $pm";
	break;
done

# who am i?
need_sudo=1
if [ `whoami` = "root" ]; then
	need_sudo=0
	echo "* Logged in as root user...";
else
	echo "!! You need to login as root (or sudo) !!";
fi

if [ need_sudo ]
	apt_32 = "sudo " + apt_32
if

#
# Install Deps
#
if [ platform == "linux" ]
	if [ is_64_bit == 0]
# LINUX 32b start
	echo ">> Installing Linux 32 bit deps";
		if [ package_manager==aptitude ]
		fi
# LINUX 32b end
else
# LINUX 64b start
	echo ">> Installing Linux 64 bit deps";
# LINUX 64b end
fi
elif [ platform == "osx" ]
# MACOSX start
	echo ">> Installing MACOSX deps";

# MACOSX end
fi
