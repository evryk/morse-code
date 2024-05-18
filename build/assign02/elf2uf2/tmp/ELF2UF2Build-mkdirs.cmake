# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/evachauffour/pico/pico-sdk/tools/elf2uf2"
  "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/elf2uf2"
  "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2"
  "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2/tmp"
  "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2/src/ELF2UF2Build-stamp"
  "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2/src"
  "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2/src/ELF2UF2Build-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2/src/ELF2UF2Build-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/evachauffour/Desktop/3rd year/Semester 2/Microprocessor Systems II/assignment-2-micro/build/assign02/elf2uf2/src/ELF2UF2Build-stamp${cfgdir}") # cfgdir has leading slash
endif()
