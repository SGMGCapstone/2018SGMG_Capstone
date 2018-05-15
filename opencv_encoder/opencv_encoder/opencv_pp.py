import cv2
import os
import pp
import sys


from time import sleep
from moviepy.editor import VideoFileClip, concatenate_videoclips
class video_slice:
    mImage = []
 
    def __init__(self, images, start, end):
        for i in range(start, end):
            self.mImage.append(images[i]);

def make_video(images, outimg=None, fps=20, size=None, is_color=True, format="MP4V", outvid='image_video.mp4'):
        from cv2 import VideoWriter, VideoWriter_fourcc, imread, resize
        fourcc = VideoWriter_fourcc(*format)
        vid = None
        for image in images:
            if not os.path.exists(image):
                raise FileNotFoundError(image)
            img = imread(image)
            if vid is None:
                if size is None:
                    size = img.shape[1], img.shape[0]
                vid = VideoWriter(outvid, fourcc, float(fps), size, is_color)
            if size[0] != img.shape[1] and size[1] != img.shape[0]:
                img = resize(img, size)
            vid.write(img)
        vid.release()
        return vid
def concate_video(name,start):
    _images = []
    for i in range(start,start+100):
         _name = name + ('%03d' % i) + '.jpg'
        # print(_name)
         _images.append(_name)
    make_video(_images,outvid= ('%01d' %((start+100)/100))+'.mp4')
    return ('%01d' %((start+100)/100))+'.mp4'


ppservers = ("192.168.0.16:8080",)

if len(sys.argv) > 1:
    ncpus = int(sys.argv[1])
    # Creates jobserver with ncpus workers
    job_server = pp.Server(ncpus, ppservers=ppservers)
else:
    # Creates jobserver with automatically detected number of workers
    job_server = pp.Server(ppservers=ppservers)

print("Starting pp with %s workers" % job_server.get_ncpus())


inputs = (1,101,201)

jobs = [(input, job_server.submit(concate_video, ('images/rc_D07_11_GB1_fsoo=',input, ), (make_video, ),
        ("cv2", ))) for input in inputs]

for input, job in jobs:
    print("encodeing %s is %s" % (input, job()))

    job_server.print_stats()
"""
concate_video('images/rc_D07_11_GB1_fsoo=',1,'1')
concate_video('images/rc_D07_11_GB1_fsoo=',101,'2')
concate_video('images/rc_D07_11_GB1_fsoo=',201,'3')

"""
clip1 = VideoFileClip("1.mp4")
clip2 = VideoFileClip("2.mp4")
clip3 = VideoFileClip("3.mp4")
final_clip = concatenate_videoclips([clip1,clip2,clip3])
final_clip.write_videofile("my_concatenation.mp4")
