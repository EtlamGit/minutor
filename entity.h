/*
   Copyright (c) 2013, Sean Kasun
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef ENTITY_H
#define ENTITY_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <QPainter>
class Tag;


class Entity
{
public:
	Entity(double nx = 0, double ny = 0, double nz = 0, QString nid = "");
	Entity(double nx, double ny, double nz, QString nid, QVariant nproperties);

	enum ECAT { ET_OTHER, ET_HOSTILE, ET_PASSIVE, ET_ITEM };
	
	bool  load(Tag *t);

	const QString&  getId()         const { return id; }
	const QVariant& getProperties() const { return properties; }
	Entity::ECAT    getCatergory()  const { return category; }
	double getX() const { return x; }
	double getY() const { return y; }
	double getZ() const { return z; }

	void draw(double offsetX, double offsetZ, double scale, QPainter& canvas) const;
	
	bool intersects(double x1, double y1, double z1,
                    double x2, double y2, double z2) const;
	
private:
    void     defineCategory();

	double   x, y, z;
	QString  id;
	ECAT     category;
	QVariant properties;
	QColor   color;
};

//           type
typedef QMap<Entity::ECAT, QList< Entity > > TEntityMap;

#endif // ENTITY_H
