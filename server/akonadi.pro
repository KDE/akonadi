SUBDIRS += src
TEMPLATE = subdirs 
CONFIG += release \
          warn_on \
          qt \
          thread

QT  += network debug
