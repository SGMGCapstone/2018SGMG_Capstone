import cv2
import os

class video_slice:
    mImage = []
 
    # 초기자(initializer)
    def __init__(self, images, start, end):
        for i in range(start, end):
            self.mImage.append(images[i]);

def make_video(images, outimg=None, fps=20, size=None, is_color=True, format="MJPG", outvid='image_video.avi'):
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

_images = []

for i in range(1, 565):
    _name = 'images/rc_D07_11_GB1_fsoo=' + ('%03d' % i) + '.jpg'
    print(_name)
    _images.append(_name)
    
make_video(_images)
