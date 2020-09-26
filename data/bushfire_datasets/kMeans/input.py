import numpy as np
from scipy.io import loadmat, savemat
import glob
import os
import re
import pdb


class Input():
    def __init__(self, data_dir, train_images, test_images, i_channels=list(range(100))):
        self.data_dir = data_dir
        self.train_images = train_images
        self.test_images = test_images
        self.i_channels = i_channels
        self._process()

    def _process(self):
        self._load_images()
        self._create_train_test_pixels()

    def _list_files_by_regex(self, regex):
        files = os.listdir(self.data_dir)
        files = list(filter(lambda x: re.match(regex, x), files))
        files = list(map(lambda x: self.data_dir+"/"+x, files))
        return files

    def _normalize(self, data):
        idxs = np.argwhere(np.isnan(data))
        data[idxs[:,0], idxs[:,1], idxs[:,2], idxs[:,3]] = 0
        idxs = np.argwhere(np.isinf(data))
        data[idxs[:, 0], idxs[:, 1], idxs[:, 2], idxs[:,3]] = 0

    def _load_images(self):
        files = glob.glob(self.data_dir + "/*.mat")
        # train_files = glob.glob(self.data_dir + "/" + self.train_images + ".mat")
        train_files = self._list_files_by_regex(self.train_images)
        # test_files = glob.glob(self.data_dir + "/" + self.test_images + ".mat")
        test_files = self._list_files_by_regex(self.test_images)

        train_images = []
        test_images = []
        for file in train_files:
            print(file)
            mat = loadmat(file)
            image = mat['image'][:, :, self.i_channels]
            train_images.append(image)
        for file in test_files:
            mat = loadmat(file)
            image = mat['image'][:, :, self.i_channels]
            test_images.append(image)
        self.train_files = train_files
        self.test_files = test_files
        self.train_images = np.array(train_images, dtype=np.float32)
        self.test_images = np.array(test_images, dtype=np.float32)
        self._normalize(self.train_images)
        self._normalize(self.test_images)

    def _create_train_test_pixels(self):
        n, height, width, channels = self.train_images.shape
        self.train_pixels = self.train_images.reshape(-1, channels)
        self.test_pixels = self.test_images.reshape(-1, channels)

    def pixels2images(self, predict, kind="test"):
        if kind == "test":
            n, height, width, channels = self.test_images.shape
            return predict.reshape(n, height, width)
        elif kind == "train":
            n, height, width, channels = self.train_images.shape
            return predict.reshape(n, height, width)
