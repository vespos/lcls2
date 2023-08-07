
"""
Class :py:class:`GWViewImageROI` is a GWView for interactive image
====================================================================

GWView <- QGraphicsView <- ... <- QWidget

Usage ::

    # Test
    #-----
    import sys
    from psana.graphqt.GWViewImageROI import *
    import psana.graphqt.ColorTable as ct
    app = QApplication(sys.argv)
    ctab = ct.color_table_monochr256()
    w = GWViewImageROI(None, arr, origin='UL', scale_ctl='HV', coltab=ctab)
    w.show()
    app.exec_()

    # Main methods in addition to GWView
    #------------------------------------
    w.set_pixmap_from_arr(arr, set_def=True)
    w.set_coltab(self, coltab=ct.color_table_rainbow(ncolors=1000, hang1=250, hang2=-20))

    w.connect_mouse_press_event(w.test_mouse_press_event_reception)
    w.connect_mouse_move_event(w.test_mouse_move_event_reception)
    w.connect_scene_rect_changed(w.test_scene_rect_changed_reception)

    # Methods
    #--------
    w.set_style()
    ix, iy, v = w.cursor_on_image_pixcoords_and_value(p)

    # Call-back slots
    #----------------
    w.mousePressEvent(e)
    # w.mouseMoveEvent(e)
    # w.closeEvent(e)
    w.key_usage()
    w.keyPressEvent(e)

    # Overrides method from GWView
    #-----------------------------
    w.test_mouse_move_event_reception(e) # signature differs from GWView

    # Global methods for test
    #------------------------
    img = image_with_random_peaks(shape=(500, 500))

See:
    - :class:`GWView`
    - :class:`GWViewImage`
    - :class:`QWSpectrum`
    - `lcls2 on github <https://github.com/slac-lcls/lcls2>`_.

This software was developed for the LCLS2 project.
If you use all or part of it, please give an appropriate acknowledgment.

Created on 2016-09-09 by Mikhail Dubrovin
Adopted for LCLS2 on 2018-02-16
"""

from psana.graphqt.GWViewImage import *
from psana.pyalgos.generic.NDArrUtils import np, info_ndarr
from PyQt5.QtGui import  QPen, QPainter, QColor, QBrush, QTransform, QPolygonF
from PyQt5.QtCore import Qt, QPoint, QPointF, QRect, QRectF, QSize, QSizeF, QLineF
import psana.graphqt.QWUtils as gu
import psana.graphqt.GWROIUtils as roiu
QPEN_DEF, QBRUSH_DEF, QCOLOR_DEF, QCOLOR_SEL, QCOLOR_EDI =\
roiu.QPEN_DEF, roiu.QBRUSH_DEF, roiu.QCOLOR_DEF, roiu.QCOLOR_SEL, roiu.QCOLOR_EDI

FNAME_DEF = 'roi_parameters.json'


def info_dict(d, sep='\n  ', indent='  '):
    return indent + sep.join(['%s: %s'%(str(k).rjust(4),v) for k,v in d.items()])

def save_dict_in_json_file(d, fname=FNAME_DEF):
    import json
    with open(fname, 'w') as f: json.dump(d, f)

def load_dict_from_json_file(fname=FNAME_DEF):
    import json
    with open(fname, 'r') as f: data = f.read()
    return json.loads(data)


class GWViewImageROI(GWViewImage):

    image_pixmap_changed = pyqtSignal()

    def __init__(self, parent=None, arr=None,\
                 coltab=ct.color_table_rainbow(ncolors=1000, hang1=250, hang2=-20),\
                 origin='UL', scale_ctl='HV', show_mode=0, signal_fast=True):

        GWViewImage.__init__(self, parent, arr, coltab, origin, scale_ctl, show_mode, signal_fast)
        self._name = 'GWViewImageROI' # self.__class__.__name__
        self.set_roi_and_mode()
        self.list_of_rois = []
        self._iscpos_old = None
        self.left_is_pressed = False
        self.right_is_pressed = False
        self.roi_active = None
        self.clicknum = 0
        #self.set_style_focus()

#    def set_style_focus(self):
#        #    QGraphicsView::item:focus {
#        self.style = """
#            QGraphicsRectItem:focus {
#                background: red;
#                selection-background-color: green;
#                border: 1px solid gray;}
#            QGraphicsRectItem:focus {border-color: blue;}"""
#        self.setStyleSheet(self.style)

#    def paintEvent(self, e):
#        print('XXX in paintEvent') # , dir(e))
#        GWViewImage.paintEvent(self, e)


    def set_roi_and_mode(self, roi_type=roiu.NONE, mode_type=roiu.NONE):
        self.roi_type = roi_type
        self.roi_name = roiu.dict_mode_type_name[roi_type]
        self.mode_type = mode_type
        self.mode_name = roiu.dict_mode_type_name[mode_type]


    def mousePressEvent(self, e):
        GWViewImage.mousePressEvent(self, e)

        #logger.info('GWViewImageROI.mousePressEvent but=%d (1/2/4 = L/R/M) screen x=%.1f y=%.1f'%(e.button(), e.pos().x(), e.pos().y()))

        self.left_is_pressed  = e.button() == Qt.LeftButton
        self.right_is_pressed = e.button() == Qt.RightButton
        #self.mid_is_pressed   = e.button() == Qt.MidButton

        if not self.left_is_pressed:
            logging.warning('USE LEFT MOUSE BUTTON ONLY !!! - other buttons finishes input or edition')
            self.finish()
            return

        if   self.mode_type < roiu.ADD: return
        elif self.mode_type & roiu.ADD:    self.on_press_add(e)
        elif self.mode_type & roiu.REMOVE: self.on_press_remove(e)
        elif self.mode_type & roiu.SELECT: self.on_press_select(e)
        elif self.mode_type & roiu.EDIT:   self.on_press_edit(e)


    def mouseMoveEvent(self, e):
        GWViewImage.mouseMoveEvent(self, e)
        if   self.mode_type < roiu.ADD: return
        elif self.mode_type & roiu.ADD:    self.on_move_add(e)
        elif self.mode_type & roiu.REMOVE: self.on_move_remove(e)
        elif self.mode_type & roiu.SELECT: self.on_move_select(e)
        elif self.mode_type & roiu.EDIT:   self.on_move_edit(e)


    def mouseReleaseEvent(self, e):
        GWViewImage.mouseReleaseEvent(self, e)
        logger.debug('mouseReleaseEvent')
        self.left_is_pressed = False
        self.right_is_pressed = False


    def set_roi_color(self, roi=None, color=QCOLOR_DEF):
        """sets roi color, by default for self.roi_active sets QCOLOR_DEF"""
        o = self.roi_active if roi is None else roi
        if o is not None:
            pen = o.scitem.pen() if hasattr(o.scitem, 'pen') else QPEN_DEF
            pen.setColor(color)
            pen.setCosmetic(True)
            o.scitem.setPen(pen)
            if self.roi_type in (roiu.PIXEL, roiu.PIXGROUP):
                brush = o.scitem.brush() if hasattr(o.scitem, 'brush') else QBRUSH_DEF
                brush.setColor(color)
                o.scitem.setBrush(brush)


    def finish(self):
        if self.mode_type & roiu.ADD:
            self.finish_add_roi()
        if self.mode_type & roiu.EDIT:
            self.finish_edit_roi()


    def finish_add_roi(self):
        """finish add_roi action on or in stead of last click, set poly at last non-closing click"""
        logger.debug('finish_add_roi')
        if self.roi_active is not None:
           self.roi_active.finish_add_roi() # used in ROIPolygon
        self.roi_active = None


    def finish_edit_roi(self):
        self.set_roi_color()
        self.roi_active = None


    def rois_at_point(self, p):
        """retiurns list of ROI object found at QPointF p"""
        items = self.scene().items(p)
        rois = [o for o in self.list_of_rois if o.scitem in items]
        logger.debug('select_roi list of ROIs at point: %s' % str(rois))
        return rois


    def one_roi_at_point(self, p):
        rois = self.rois_at_point(p)
        s = len(rois)
        if s<1: return None
        elif self.roi_type == roiu.PIXEL: # select pixel rect nearest to the click position
            dmins = [(p - o.scitem.rect().center()).manhattanLength() for o in rois]
            return rois[dmins.index(min(dmins))]
        else:
            return rois[0]


    def scene_pos(self, e):
        """scene position for mouse event"""
        return self.mapToScene(e.pos())


    def on_press_select(self, e, color_sel=QCOLOR_SEL):
        """select ROI on mouthPressEvent"""
        logger.debug('GWViewImageROI.on_press_select at scene pos: %s' % str(self.scene_pos(e)))
        o = self.one_roi_at_point(self.scene_pos(e))
        if o is None: return
        color = QCOLOR_DEF if o.scitem.pen().color() == color_sel else color_sel
        self.set_roi_color(o, color)


    def select_roi_edit(self, e, color_edi=QCOLOR_EDI):
        self.roi_active = self.one_roi_at_point(self.scene_pos(e))


    def deselect_any_edit(self):
        for o in self.list_of_rois:
            if o.scitem.pen().color() == QCOLOR_EDI:
                o.hide_handles()
            if o.scitem.pen().color() != QCOLOR_DEF:
                self.set_roi_color(o, QCOLOR_DEF)


    def on_press_edit(self, e, color_edi=QCOLOR_EDI):
        self.select_roi_edit(e, color_edi)
        if self.roi_active is None:
            logger.warning('ROI FOR EDIT IS NOT FOUND near scene point %s' % str(self.scene_pos(e)))
            return
        self.deselect_any_edit()
        self.set_roi_color(self.roi_active, QCOLOR_EDI)
        self.roi_active.show_handles()


    def remove_roi(self, o):
        """remove roi item from scene and from the list of rois."""
        self.scene().removeItem(o.scitem)
        self.list_of_rois.remove(o)


    def on_move_remove(self, e):
        """remove ROI on mouthPressEvent - a few ROIs might be removed"""
        if self.left_is_pressed:
            logger.debug('GWViewImageROI.remove_roi for non-PIXEL types')
            items = self.scene().items(self.scene_pos(e))
            roisel = [o for o in self.list_of_rois if o.scitem in items]
            logger.debug('remove_roi list of ROIs at point: %s' % str(roisel))
            for o in roisel:
                self.remove_roi(o)


    def delete_selected_roi(self):
        """delete all ROIs selected/marked by QCOLOR_SEL"""
        roisel = [o for o in self.list_of_rois if o.scitem.pen().color() == QCOLOR_SEL]
        logger.debug('delete_selected_roi: %s' % str(roisel))
        for o in roisel:
            self.remove_roi(o)


    def delete_all_roi(self):
        """delete all ROIs"""
        logger.debug('delete_all_roi')
        sc = self.scene()
        for o in self.list_of_rois:
            sc.removeItem(o.scitem)
        self.list_of_rois = []


    def cancel_add_roi(self):
        """cancel of adding item to scene"""
        o = self.roi_active
        if o is None:
            logger.debug('GWViewImageROI.cancel_add_roi roi_active is None - nothing to cancel...')
            return
        self.scene().removeItem(o.scitem)
        self.list_of_rois.remove(o)
        self.roi_active = None


    def on_press_remove(self, e):
        logger.info('GWViewImageROI.on_press_remove DEPRICATED - use SELECT then DELETE')


    def on_press_add(self, e):
        scpos = self.scene_pos(e)

        if self.roi_active is None: #  1st click
            logger.debug('on_press_add - 1-st click')
            self.clicknum = 1
            self.add_roi(e)

        else: # other clicks
            self.clicknum += 1
            logger.debug('on_press_add - click %d' % self.clicknum)
            self.roi_active.set_point_at_add(scpos, self.clicknum)
            if self.roi_active.is_last_point(scpos, self.clicknum):
                self.finish_add_roi()


    def pixel_is_removed(self, iscpos):
        #if self.mode_type & roiu.REMOVE:
        if self.roi_type == roiu.PIXEL:
            for o in self.list_of_rois:
                if iscpos == o.pos:
                    self.remove_roi(o)
                    return True
        return False


    def add_roi(self, e):
        scpos = self.scene_pos(e)
        iscpos = roiu.int_scpos(scpos)
        is_busy = any([iscpos == o.pos for o in self.list_of_rois])
        logger.debug('GWViewImageROI.add_roi scene ix=%d iy=%d is_busy=%s'%(iscpos.x(), iscpos.y(), is_busy))

        if is_busy and self.pixel_is_removed(iscpos): return

        o = roiu.create_roi(self.roi_type, view=self, pos=scpos, is_busy_iscpos=is_busy)
        if o is None:
            logger.warning('ROI of type %d is undefined' % self.roi_type) # roiu.dict_roi_type_name[self.roi_type])
            return
        o.add_to_scene(pos=scpos)
        if o.scitem is not None:
            self.list_of_rois.append(o)
            self.roi_active = o
        if self.roi_active is not None\
        and self.roi_active.is_last_point(scpos, self.clicknum):
            self.finish_add_roi()


    def on_move_edit(self, e):
        logger.debug('on_move_edit TBD')


    def on_move_select(self, e):
        logger.debug('on_move_select TBD')


    def on_move_add(self, e):
        """Action on mouse move:
           if mouse position is in the same pixel as in previous event - do nothing
           if roi type is PIXEL - try to add/remove pixel
           for other roi_type call roi_active.move_at_add
        """
        scpos = self.scene_pos(e)
        iscpos = roiu.int_scpos(scpos)
        is_same = (iscpos == self._iscpos_old)
        logger.debug('GWViewImageROI.add_roi scene ix=%d iy=%d is_same=%s'%(iscpos.x(), iscpos.y(), is_same))
        if is_same: return
        self._iscpos_old = iscpos #  QPoint(iscpos)

        if self.roi_active is None:
          if self.roi_type == roiu.PIXEL and self.left_is_pressed:
            self.add_roi(e)
        else:
          self.roi_active.move_at_add(scpos, self.left_is_pressed)


    def save_parameters_in_file(self, fname=FNAME_DEF):
        d = {'ROI_%04d'%i:o.roi_pars() for i,o in enumerate(self.list_of_rois)}
        save_dict_in_json_file(d, fname)
        logger.info('GWViewImageROI.save_parameters_in_file %s\n%s' % (fname, info_dict(d)))


    def load_parameters_from_file(self, fname=FNAME_DEF):
        d = load_dict_from_json_file(fname)
        logger.info('GWViewImageROI.load_parameters_from_file %s\n%s' % (fname, info_dict(d)))
        self.set_rois_from_dict(d)


    def set_rois_from_dict(self, dtot):
        logger.debug('GWViewImageROI.set_rois_from_dict\n%s' % info_dict(dtot))
        self.delete_all_roi()
        for k,d in dtot.items():
            logger.info('XXX set roi %4s for dict: %s' % (k,d))
            roi_type =d['roi_type']
            xy = d['points'][0]
            o = roiu.create_roi(roi_type, view=self, pos=QPointF(*xy), is_busy_iscpos=False)
            if o is None:
                logger.warning('ROI of type %d is undefined' % self.roi_type)
                continue
            if o.set_from_roi_pars(d):
                self.list_of_rois.append(o)


    def set_pixmap_from_arr(self, arr, set_def=True, amin=None, amax=None, frmin=0.00001, frmax=0.99999):
        """Input array is scailed by color table. If color table is None arr set as is."""

        GWViewImage.set_pixmap_from_arr(self, arr, set_def, amin, amax, frmin, frmax)

        image = self.qimage # QImage
        pixmap = self.qpixmap # QPixmap

        arr = ct.image_to_arrcolors(self.qimage, channels=4)
#        print(info_ndarr(arr, 'XXXX image_to_arrcolors:'))

#        arrB = ct.pixmap_channel(self.qpixmap, channel=0)
#        arrG = ct.pixmap_channel(self.qpixmap, channel=1)
#        arrR = ct.pixmap_channel(self.qpixmap, channel=2)
        arrA = ct.pixmap_channel(self.qpixmap, channel=3)

#        print(info_ndarr(arrA, 'XXXX alpha channel'))

        #mask = pixmap.createMaskFromColor()
        #arr = pixmap_to_arrcolors(pixmap, channels=4)

if __name__ == "__main__":
    import sys
    sys.exit(qu.msg_on_exit())

# EOF
