// Copyright (c) 2020, Martin Sandsmark <martin.sandsmark@kde.org>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "../../include/litehtml.h"
#include <QPainter>
#include <QImage>

struct cairo_clip_box {
    typedef std::vector<cairo_clip_box> vector;
    litehtml::position  box;
    litehtml::border_radiuses radius;

    cairo_clip_box(const litehtml::position &vBox, litehtml::border_radiuses vRad)
    {
        box = vBox;
        radius = vRad;
    }

    cairo_clip_box(const cairo_clip_box &val)
    {
        box = val.box;
        radius = val.radius;
    }
    cairo_clip_box &
    operator=(const cairo_clip_box &val)
    {
        box = val.box;
        radius = val.radius;
        return *this;
    }
};

struct cairo_font {
    cairo_font_face_t  *font;
    int                 size;
    bool                underline;
    bool                strikeout;
};

class container_qt : public litehtml::document_container
{
    typedef std::map<litehtml::tstring, Glib::RefPtr<Gdk::Pixbuf> > images_map;

protected:
    QImage            *m_temp_surface;
    QPainter                    *m_temp_cr;
    QHash<QUrl, QImage> m_images;
    QRegion      m_clips;
public:
    container_qt(void);
    virtual ~container_qt(void);

    virtual litehtml::uint_ptr          create_font(const litehtml::tchar_t *faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics *fm) override;
    virtual void                        delete_font(litehtml::uint_ptr hFont) override;
    virtual int                     text_width(const litehtml::tchar_t *text, litehtml::uint_ptr hFont) override;
    virtual void                        draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t *text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position &pos) override;
    virtual int                     pt_to_px(int pt) override;
    virtual int                     get_default_font_size() const override;
    virtual const litehtml::tchar_t    *get_default_font_name() const override;
    virtual void                        load_image(const litehtml::tchar_t *src, const litehtml::tchar_t *baseurl, bool redraw_on_ready) override;
    virtual void                        get_image_size(const litehtml::tchar_t *src, const litehtml::tchar_t *baseurl, litehtml::size &sz) override;
    virtual void                        draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint &bg) override;
    virtual void                        draw_borders(litehtml::uint_ptr hdc, const litehtml::borders &borders, const litehtml::position &draw_pos, bool root) override;
    virtual void                        draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker) override;
    virtual std::shared_ptr<litehtml::element>  create_element(const litehtml::tchar_t *tag_name,
            const litehtml::string_map &attributes,
            const std::shared_ptr<litehtml::document> &doc) override;
    virtual void                        get_media_features(litehtml::media_features &media) const override;
    virtual void                        get_language(litehtml::tstring &language, litehtml::tstring &culture) const override;
    virtual void                        link(const std::shared_ptr<litehtml::document> &ptr, const litehtml::element::ptr &el) override;


    virtual void                        transform_text(litehtml::tstring &text, litehtml::text_transform tt) override;
    virtual void                        set_clip(const litehtml::position &pos, const litehtml::border_radiuses &bdr_radius, bool valid_x, bool valid_y) override;
    virtual void                        del_clip() override;

    virtual void                        make_url(const litehtml::tchar_t *url, const litehtml::tchar_t *basepath, litehtml::tstring &out);
    virtual QImage   get_image(const QUrl &url, bool redraw_on_ready) = 0;

    void                                clear_images();

protected:
    virtual void                        draw_ellipse(QPainter *cr, int x, int y, int width, int height, const litehtml::web_color &color, int line_width);
    virtual void                        fill_ellipse(QPainter *cr, int x, int y, int width, int height, const litehtml::web_color &color);
    virtual void                        rounded_rectangle(QPainter *cr, const litehtml::position &pos, const litehtml::border_radiuses &radius);

private:
    static QUrl qUrl(const litehtml::tchar_t *url, const litehtml::tchar_t *basepath) {
        if (basepath) {
            QUrl ret(QString::fromLocal8Bit(basepath));
            ret.setPath(ret.path() + QLatin1Char('/') + QString::fromLocal8Bit(url));
            qDebug() << ret;
            return ret;
        } else {
            qDebug() << QUrl(QString::fromLocal8Bit(url));
            return QUrl(QString::fromLocal8Bit(url));
        }
    }
    static QColor qColor(const litehtml::web_color &color) {
        return qRgba(color.red, color.green, color.blue, color.alpha);
    }
    static QRect qRect(const litehtml::position &rect) {
        return QRect(rect.x, rect.y, rect.width, rect.height);
    }

    void                                apply_clip(QPainter *cr);
    void                                add_path_arc(QPainter *cr, double x, double y, double rx, double ry, double a1, double a2, bool neg);
    void
    set_color(QPainter *cr, litehtml::web_color color)
    {
        QPen pen = cr->pen();
        pen.setColor(qRgba(color.red, color.green, color.blue, color.alpha));
        cr->setPen(pen);
    }
    void                                draw_pixbuf(QPainter *cr, const Glib::RefPtr<Gdk::Pixbuf> &bmp, int x, int y, int cx, int cy);
    //cairo_surface_t                    *surface_from_pixbuf(const Glib::RefPtr<Gdk::Pixbuf> &bmp);
};
