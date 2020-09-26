from models.KMeans.input import Input
import argparse
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt
import numpy as np
import pdb
import re
import os


def parse_args():
    args = argparse.ArgumentParser(description="Kmeans clustering.")
    args.add_argument("--data_dir", default="data/data_nov", help="Data directory.")
    args.add_argument(
        "--train_images", default="201811(09)(10|11|12|13|14|15|16|17|18|19|20|21)00.mat", help="Train images regex.")
    args.add_argument("--test_images", default="201811(08)(10|11|12|13|14|15|16|17|18|19|20|21)00.mat", help="Test images regex.")
    args.add_argument("--output_dir", default="visualization/algorithm_results/kmeans/",
                      help="Output directory.")
    args.add_argument("--seed", default=123, type=int)
    return args.parse_args()


def get_filename(filepath):
    elms = re.split(r"[\\\/]", filepath)
    elms = list(filter(lambda x: len(x.strip()) > 0, elms))
    return elms[-1].split(".")[0]


def run(args):

    # list_channels = [list(range(16))]
    #  list_channels += [[x] for x in range(16)]
    # list_channels += [[6, x] for x in range(16)]
    # list_channels += [list(range(6, x)) for x in range(8, 16)]
    # list_channels += [list(range(6, 16))]
    list_channels = [[2,5,6]]

    file_names = list(map(lambda x: '-'.join(list(map(str, x))), list_channels))

    for n_clusters in [3]:
        for idx, channels in enumerate(list_channels):
            print("n clusters: {} | channels: {}".format(n_clusters, channels))
            _input = Input(args.data_dir,
                           args.train_images,
                           args.test_images,
                           i_channels=channels)
            max_iter = 500
            kmeans = KMeans(n_clusters=n_clusters, random_state=args.seed,
                            max_iter=max_iter).fit(_input.train_pixels)
          
            for idx_file, file in enumerate(_input.test_files):
                predict_test = kmeans.predict(_input.test_pixels)
                predict_test = _input.pixels2images(predict_test, kind="test")
                np.savetxt(args.output_dir + '/csv/' + get_filename(file) +'.csv',predict_test[idx_file].astype(int), fmt='%i')

                fig = plt.figure(dpi=100)
                plt.imshow(predict_test[idx_file], cmap='hot', interpolation='nearest')
                if not os.path.exists("{}/{}".format(args.output_dir, "{}-clusters-channel-{}".format(n_clusters, file_names[idx]))):
                    os.makedirs("{}/{}".format(args.output_dir,
                                               "{}-clusters-channel-{}".format(n_clusters, file_names[idx])))

                plt.savefig("{}/{}/{}.jpg".format(
                    args.output_dir,
                    "{}-clusters-channel-{}".format(n_clusters, file_names[idx]),
                    get_filename(file)))
                plt.close(fig)

if __name__ == '__main__':
    args = parse_args()
    run(args)
