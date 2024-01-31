# Frisk

General use directory size comparison and overview by @kubgus.

## Installation

Run the following command in the source directory of this program:

```bash
g++ main.cpp -o frisk -Ofast; sudo mv frisk /bin/
```

If you downloaded one of the releases as an executable, navigate to where you downloaded the executable and run:

```bash
sudo mv frisk /bin/
```

## Usage

Running the command as it is frisks the current directory with default flags. (see below)

```bash
frisk
```

You can view all the flags in terminal with the `-h` flag:

```bash
frisk -h
```

### Flags

`-p`, `--path`: Specify the path to frisk. (defaults to current working directory)

`-i`, `--ignore`: Specify a comma-separated list of file/directory names to ignore when printing out the result.

> ***Note:*** Default value is `.git,node_modules`

## Contribution

Feel free to contribute anything you find valuable. The code is very simplistic.
