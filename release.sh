#!/usr/bin/env bash
# encoding: utf-8

# Creates a new release
# Updates the VERSION.txt file and creates the tag
# Kindly adapted from ET: Legacy team

set -Eeuo pipefail

# current Git branch
branch=$(git symbolic-ref HEAD | sed -e 's,.*/\(.*\),\1,')

# Require master
if [[ "$branch" != "master" ]]; then
	echo "Not in master branch, exiting!"
	exit 1
fi

# Require the current working directory to be clean
if [[ -n "$(git status --porcelain)" ]]; then
	echo "Git repository is not clean, exiting!"
	exit 1
fi

# Parse the version file
major=$(grep "VERSION_MAJOR" VERSION.txt | cut -d" " -f2)
minor=$(grep "VERSION_MINOR" VERSION.txt | cut -d" " -f2)
patch=$(grep "VERSION_PATCH" VERSION.txt | cut -d" " -f2)
version_changed=
version_message=
gpg_sign=

parse_params() {
	while :; do
		case "${1-}" in
		-v | --verbose) set -x ;;
		-h | --help)
			printf 'Usage: \n./release.sh <--major|--minor|--patch> [--sign] [-m|--message] [-v|--verbose]\n\nOptions:\n  --major    Increment major version\n  --minor    Increment minor version\n  --patch    Increment patch version\n  --sign     Sign the release using GPG key\n  --message  Adjust commit message (default "ETJump MAJOR.MINOR.PATCH")\n  --verbose  Verbose output\n'
			;;
		--major)
			major=$((major+1))
			minor=0
			patch=0
			version_changed=true
			;;
		--minor)
			minor=$((minor+1))
			patch=0
			version_changed=true
			;;
		--patch)
			patch=$((patch+1))
			version_changed=true
			;;
		--sign)
			gpg_sign=true
			;;
		-m | --message)
			version_message="${2-}"
			shift
			;;
		-?*) die "Unknown option: $1" ;;
		*) break ;;
		esac
		shift
	done

	return 0
}

parse_params "$@"

# If nothing has changed then just exit
if [[ -z $version_changed ]]; then
	echo "Version not changed, exiting!"
	exit 0
fi

# Sorry tag is already taken.
if [[ "$(git tag -l "$major.$minor.$patch")" ]]; then
	echo "Tag '$major.$minor.$patch' is already taken, exiting!"
	exit 1
fi

if [[ "$patch" = 0 ]] && [[ "$(git tag -l "$major.$minor")" ]]; then
	echo "Tag '$major.$minor' is already taken, exiting!"
	exit 1
fi

if [[ -z "$version_message" ]]; then
	version_message="ETJump $major.$minor.$patch"
fi

echo "Ready to commit and tag a new version: $major.$minor.$patch"
echo "Version message will be: $version_message"
read -p "Ready to commit? [Y/N]: " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
	# Update the version file.
	perl -pi -e "s/(VERSION_MAJOR)\s+[0-9]+/\1 $major/g" VERSION.txt
	perl -pi -e "s/(VERSION_MINOR)\s+[0-9]+/\1 $minor/g" VERSION.txt
	perl -pi -e "s/(VERSION_PATCH)\s+[0-9]+/\1 $patch/g" VERSION.txt

	if [[ -n $gpg_sign ]]; then
		# no signing
		# Create the release commit
		git commit -am "$version_message"

		# Tag it like a champ!
		git tag -a "$major.$minor.$patch" -m "$version_message"
	else
		# Create the release commit
		git commit -a -S -m "$version_message"

		# sign the tag
		git tag -s "$major.$minor.$patch" -m "$version_message"
	fi

	echo "Committed and tagged a new release"
	read -p "Enter the name of the upstream remote (default: origin): " upstream
	upstream=${upstream:-origin}

	read -p "Push commit and tag to remote '$upstream'? [Y/N]: " -n 1 -r
	echo
	if [[ $REPLY =~ ^[Yy]$ ]]; then
		git push "$upstream" HEAD
		git push "$upstream" "$major.$minor.$patch"
		echo "Pushed data to remote '$upstream'. Congrats!"
	else
		echo "You need to 'git push $upstream HEAD' and 'git push $upstream --tags' manually."
	fi
else
  echo "Chicken!"
fi
